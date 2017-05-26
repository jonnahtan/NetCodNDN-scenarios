/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "weighted-load-balancer-strategy.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/mem_fun.hpp>

//#include <ndn-cxx/util/time.hpp>

#include <boost/chrono/system_clocks.hpp>
#include <ndn-cxx/util/random.hpp>
#include "core/logger.hpp"

#include "table/measurements-entry.hpp"

// ns-3 specifics for the seed
#include "ns3/simulator.h"

using namespace ndn::time;
using namespace boost::multi_index;

namespace nfd {
namespace fw {

class MyPitInfo;
class MyMeasurementInfo;
class WeightedFace;

class WeightedFace
{
public:

  WeightedFace(const Face& face_,
               const milliseconds& delay = milliseconds(0))
    : face(face_)
    , lastDelay(delay)
  {}

  bool
  operator<(const WeightedFace& other) const
  {
    if (lastDelay == other.lastDelay)
      return face.getId() < other.face.getId();

    return lastDelay < other.lastDelay;
  }

  FaceId
  getId() const
  {
    return face.getId();
  }

  static void
  modifyWeightedFaceDelay(WeightedFace& face,
                          const ndn::time::milliseconds& delay)
  {
    face.lastDelay = delay;
  }

  const Face& face;
  ndn::time::milliseconds lastDelay;
};

///////////////////////
// PIT entry storage //
///////////////////////

class MyPitInfo : public StrategyInfo
{
public:
  MyPitInfo()
    : creationTime(system_clock::now())
  {}

  static int constexpr
  getTypeId() { return 9970; }

  system_clock::TimePoint creationTime;
};

///////////////////////////////
// Measurement entry storage //
///////////////////////////////

class MyMeasurementInfo : public StrategyInfo
{
public:

  void
  updateFaceDelay(const Face& face, const milliseconds& delay);

  void
  updateStoredNextHops(const fib::NextHopList& nexthops);

  static milliseconds
  calculateInverseDelaySum(const shared_ptr<MyMeasurementInfo>& info);

  static int constexpr
  getTypeId() { return 9971; }

public:

  struct ByDelay {};
  struct ByFaceId {};

  typedef multi_index_container<
    WeightedFace,
    indexed_by<
      ordered_unique<
        tag<ByDelay>,
        identity<WeightedFace>
        >,
      hashed_unique<
        tag<ByFaceId>,
        const_mem_fun<WeightedFace, FaceId, &WeightedFace::getId>
        >
      >
    > WeightedFaceSet;

  typedef WeightedFaceSet::index<ByDelay>::type WeightedFaceSetByDelay;
  typedef WeightedFaceSet::index<ByFaceId>::type WeightedFaceSetByFaceId;

  //Collection of Faces sorted by delay
  WeightedFaceSet weightedFaces;

  ndn::time::milliseconds totalDelay;
};

/////////////////////////////
// Strategy Implementation //
/////////////////////////////

NFD_LOG_INIT("WeightedLoadBalancerStrategy");

const Name WeightedLoadBalancerStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/weighted-load-balancer");
NFD_REGISTER_STRATEGY(WeightedLoadBalancerStrategy);

WeightedLoadBalancerStrategy::WeightedLoadBalancerStrategy(Forwarder& forwarder,
                                                           const Name& name)
  : Strategy(forwarder, name)
{
    m_randomGenerator.seed(ns3::Simulator::GetContext());
}

WeightedLoadBalancerStrategy::~WeightedLoadBalancerStrategy()
{
}

void
WeightedLoadBalancerStrategy::afterReceiveInterest(const Face& inFace,
                                                   const Interest& interest,
                                                   shared_ptr<fib::Entry> fibEntry,
                                                   shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_TRACE("Received Interest: " << interest.getName());

  // not a new Interest, don't forward
  if (pitEntry->hasUnexpiredOutRecords())
    {
      NFD_LOG_TRACE("Not a new Interest, " << interest.getName() << ", refusing to forward");
      return;
    }

  // create timer information and attach to PIT entry
  pitEntry->setStrategyInfo<MyPitInfo>(make_shared<MyPitInfo>());

  shared_ptr<MyMeasurementInfo> measurementsEntryInfo =
           myGetOrCreateMyMeasurementInfo(fibEntry);


  // reconcile differences between incoming nexthops and those stored
  // on our custom measurement entry info
  measurementsEntryInfo->updateStoredNextHops(fibEntry->getNextHops());

  if (!this->mySendInterest(interest, measurementsEntryInfo, pitEntry))
    {
      this->rejectPendingInterest(pitEntry);
      BOOST_ASSERT(false);
    }
}


void
WeightedLoadBalancerStrategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                                                    const Face& inFace,
                                                    const Data& data)
{
  NFD_LOG_TRACE("Received Data: " << data.getName());
  shared_ptr<MyPitInfo> pitInfo = pitEntry->getStrategyInfo<MyPitInfo>();

  // No start time available, cannot compute delay for this retrieval
  if (!static_cast<bool>(pitInfo))
    {
      NFD_LOG_TRACE("No start time available for Data " << data.getName());
      return;
    }

  const milliseconds delay =
    duration_cast<milliseconds>(system_clock::now() - pitInfo->creationTime);

  NFD_LOG_TRACE("Computed delay of: " << system_clock::now() << " - " << pitInfo->creationTime << " = " << delay);

  MeasurementsAccessor& accessor = this->getMeasurements();

  // Update Face delay measurements and entry lifetimes owned
  // by this strategy while walking up the NameTree
  shared_ptr<measurements::Entry> measurementsEntry = accessor.get(*pitEntry);
  if (static_cast<bool>(measurementsEntry))
    {
      NFD_LOG_TRACE("accessor returned measurements entry " << measurementsEntry->getName()
                   << " for " << pitEntry->getName());
    }
  else
    {
      NFD_LOG_WARN ("accessor returned invalid measurements entry for " << pitEntry->getName());
    }

  while (static_cast<bool>(measurementsEntry))
    {
      shared_ptr<MyMeasurementInfo> measurementsEntryInfo =
        measurementsEntry->getStrategyInfo<MyMeasurementInfo>();

      if (static_cast<bool>(measurementsEntryInfo))
        {
          accessor.extendLifetime(*measurementsEntry, seconds(16));
          measurementsEntryInfo->updateFaceDelay(inFace, delay);
        }

      measurementsEntry = accessor.getParent(*measurementsEntry);
    }
}


void
WeightedLoadBalancerStrategy::beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  MeasurementsAccessor& accessor = this->getMeasurements();
  shared_ptr<measurements::Entry> measurementsEntry = accessor.get(*pitEntry);

  while (static_cast<bool>(measurementsEntry))
    {
      shared_ptr<MyMeasurementInfo> measurementsEntryInfo =
        measurementsEntry->getStrategyInfo<MyMeasurementInfo>();

      if (static_cast<bool>(measurementsEntryInfo))
        {
          for (auto& entry : pitEntry->getOutRecords())
            {
              measurementsEntryInfo->updateFaceDelay(*(entry.getFace()),
                                                     milliseconds::max());
            }
        }

      measurementsEntry = accessor.getParent(*measurementsEntry);
    }
}


/////////////////////////////
// Strategy Helper Methods //
/////////////////////////////

bool
WeightedLoadBalancerStrategy::mySendInterest(const Interest& interest,
                                             shared_ptr<MyMeasurementInfo>& measurementsEntryInfo,
                                             shared_ptr<pit::Entry>& pitEntry)
{
  typedef MyMeasurementInfo::WeightedFaceSetByDelay WeightedFaceSetByDelay;

  const WeightedFaceSetByDelay& facesByDelay =
    measurementsEntryInfo->weightedFaces.get<MyMeasurementInfo::ByDelay>();

  const milliseconds totalDelay = measurementsEntryInfo->totalDelay;
  const milliseconds inverseTotalDelay =
    MyMeasurementInfo::calculateInverseDelaySum(measurementsEntryInfo);
    
  std::uniform_int_distribution<uint64_t> dist(0, inverseTotalDelay.count());
  const uint64_t selection = dist(m_randomGenerator);

  NFD_LOG_TRACE("selection = " << selection);

  uint64_t cumulativeWeight = 0;

  for (WeightedFaceSetByDelay::const_iterator i = facesByDelay.begin();
       i != facesByDelay.end();
       ++i)
    {
      NFD_LOG_TRACE("cumulative weight = " << cumulativeWeight);
      // weight = inverted delay measurement
      const uint64_t weight = totalDelay.count() - i->lastDelay.count();
      cumulativeWeight += weight;

      if(selection <= cumulativeWeight && pitEntry->canForwardTo(i->face))
        {
          NFD_LOG_TRACE("fowarding " << interest.getName() << " out face " << i->face.getId());
          this->sendInterest(pitEntry, this->getFace(i->face.getId()));
          return true;
        }
    }
  return false;
}


shared_ptr<MyMeasurementInfo>
WeightedLoadBalancerStrategy::myGetOrCreateMyMeasurementInfo(const shared_ptr<fib::Entry>& entry)
{
  BOOST_ASSERT(static_cast<bool>(entry));

  //this could return null?
  shared_ptr<measurements::Entry> measurementsEntry =
    this->getMeasurements().get(*entry);

  BOOST_ASSERT(static_cast<bool>(measurementsEntry));

  shared_ptr<MyMeasurementInfo> measurementsEntryInfo =
    measurementsEntry->getStrategyInfo<MyMeasurementInfo>();

  if (!static_cast<bool>(measurementsEntryInfo))
    {
      measurementsEntryInfo = make_shared<MyMeasurementInfo>();
      measurementsEntry->setStrategyInfo(measurementsEntryInfo);
    }

  return measurementsEntryInfo;
}

///////////////////////////////////////
// MyMeasurementInfo Implementations //
///////////////////////////////////////

void
MyMeasurementInfo::updateFaceDelay(const Face& face, const milliseconds& delay)
{
  WeightedFaceSetByFaceId& facesById = weightedFaces.get<MyMeasurementInfo::ByFaceId>();

  WeightedFaceSetByFaceId::iterator faceEntry = facesById.find(face.getId());

  if (faceEntry != facesById.end())
    {
      totalDelay += (delay - faceEntry->lastDelay);

      // NFD_LOG_TRACE("Recording delay of " << delay.count()
      //               << "ms (diff: " << (delay - faceEntry->lastDelay).count()
      //               << "ms) for FaceId: " << inFace.getId());

      facesById.modify(faceEntry,
                       bind(&WeightedFace::modifyWeightedFaceDelay,
                            _1, boost::cref(delay)));
    }
}

void
MyMeasurementInfo::updateStoredNextHops(const fib::NextHopList& nexthops)
{
  WeightedFaceSetByFaceId& facesById =
    weightedFaces.get<MyMeasurementInfo::ByFaceId>();

  std::set<FaceId> nexthopFaceIds;

  for (fib::NextHopList::const_iterator i = nexthops.begin();
       i != nexthops.end();
       ++i)
    {
      const FaceId id = i->getFace()->getId();
      if (facesById.find(id) == facesById.end())
        {
          // new nexthop, add to set
          facesById.insert(WeightedFace(*i->getFace()));

          NFD_LOG_TRACE("added FaceId: " << id);
        }
      nexthopFaceIds.insert(id);
    }

  for (WeightedFaceSetByFaceId::const_iterator i = facesById.begin();
       i != facesById.end();
       ++i)
    {
      if (nexthopFaceIds.find(i->getId()) == nexthopFaceIds.end())
        {
          NFD_LOG_TRACE("pruning FaceId: " << i->getId());
          facesById.erase(i);
        }
    }
}

milliseconds
MyMeasurementInfo::calculateInverseDelaySum(const shared_ptr<MyMeasurementInfo>& info)
{
  const MyMeasurementInfo::WeightedFaceSetByDelay& facesByDelay =
    info->weightedFaces.get<MyMeasurementInfo::ByDelay>();

  milliseconds inverseTotalDelay(0);

  for (MyMeasurementInfo::WeightedFaceSetByDelay::const_iterator i = facesByDelay.begin();
       i != facesByDelay.end();
       ++i)
    {
      inverseTotalDelay += (info->totalDelay - i->lastDelay);
    }

  return inverseTotalDelay;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// NETWORK CODING ////////////////////////////////////////////////////////////////////////////

static bool
canForwardToNextHop_NetworkCoding(shared_ptr<ncft::Entry> ncftEntry, const fib::NextHop& nexthop)
{
  return ncftEntry->canForwardTo(*nexthop.getFace());
}

static bool
hasFaceForForwarding_NetworkCoding(const fib::NextHopList& nexthops, shared_ptr<ncft::Entry>& ncftEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop_NetworkCoding, ncftEntry, _1))
         != nexthops.end();
}

void
WeightedLoadBalancerStrategy::afterReceiveInterest_NetworkCoding( const Face& inFace, 
                                                                const Interest& interest,
                                                                shared_ptr<fib::Entry> fibEntry,
                                                                shared_ptr<ncft::Entry> ncftEntry)
{
  NFD_LOG_TRACE("afterReceiveInterest");

  // Check if this Interest should be forwarded or aggregated
  if (ncftEntry->isAggregating(inFace))
	{	
    // Get the total number of interets forwarded and pending for this node (sigma^{p}_{fwd}).
    uint32_t interestsForwarded = ncftEntry->getInterestsForwarded();

    // Get the number of pending interets for inFace (sigma^{p,f}_{pend}).
    uint32_t interetsPending_inFace = ncftEntry->getInterestsPending(inFace);

    // Check if this Interest should be aggregated (sigma^{p}_{fwd} > sigma^{p,f}_{pend})
    if (interestsForwarded >= interetsPending_inFace)
    {
    	// Interest will be aggregated
		  return;
    }
  }

  // List of next hops according to FIB
  fib::NextHopList nextHops = fibEntry->getNextHops();
  //fib::NextHopList& nextHops

  // Ensure there is at least 1 Face is available for forwarding
  if (!hasFaceForForwarding_NetworkCoding(nextHops, ncftEntry)) {
    this->rejectPendingInterest_NetworkCoding(ncftEntry);
    return;
  }

  // Congestion map
  std::map<uint32_t,uint32_t> congestionMap;

  // List of OutRecords
  const ncft::OutRecordCollection& outRecords = ncftEntry->getOutRecords();

  // Check the congestion in each Out Record
  for (ncft::OutRecordCollection::const_iterator outRecord = outRecords.begin();
    outRecord != outRecords.end(); ++outRecord) 
  {
    congestionMap.emplace(outRecord->getFace()->getId(), outRecord->getInterestsForwarded());
  }

  // If a face available for forwarding at FIB had no Out Record, add congestion 0
  for (fib::NextHopList::iterator nextHop = nextHops.begin(); 
    nextHop != nextHops.end(); ++nextHop) 
  {
    if (congestionMap.count(nextHop->getFace()->getId()) == 0)
    {
      congestionMap.emplace(nextHop->getFace()->getId(), 0);
    }
  }

  // Find the lowest value of pending interests
  auto l = std::min_element(congestionMap.begin(), congestionMap.end(),
    [] (std::pair<uint32_t,uint32_t> i, std::pair<uint32_t,uint32_t> j) 
    { return i.second < j.second; });

  uint32_t lowest = l->second;

  // Remove all next hops that have more than 'lowest' pending interests
  for (fib::NextHopList::iterator nextHop = nextHops.begin(); 
    nextHop != nextHops.end(); ) 
  {
    if (congestionMap.at(nextHop->getFace()->getId()) != lowest)
    {
      nextHop = nextHops.erase(nextHop);
    } 
    else
    {
      ++nextHop;
    }
  }  

  fib::NextHopList::iterator selected;
  do {
    std::uniform_int_distribution<> dist(0, nextHops.size() - 1);
    const size_t randomIndex = dist(m_randomGenerator);

    uint64_t currentIndex = 0;

    for (selected = nextHops.begin(); selected != nextHops.end() && currentIndex != randomIndex;
         ++selected, ++currentIndex) {
    }
  } while (!canForwardToNextHop_NetworkCoding(ncftEntry, *selected));

  this->sendInterest_NetworkCoding(ncftEntry, selected->getFace());
}

} // namespace fw
} // namespace nfd
