/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"


#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/coarsening/HeavyEdgeCoarsener_TestFixtures.h"
#include "partition/coarsening/HyperedgeCoarsener.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/Rater.h"

using::testing::UnorderedElementsAre;

using datastructure::verifyEquivalenceWithPartitionInfo;
using defs::Hypergraph;

namespace partition {
using HyperedgeCoarsenerType = HyperedgeCoarsener<EdgeWeightDivMultPinWeight>;

class AHyperedgeCoarsener : public Test {
 public:
  explicit AHyperedgeCoarsener(Hypergraph* graph =
                                 new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                                                HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    config(),
    coarsener(*hypergraph, config,  /* heaviest_node_weight */ 1),
    refiner(new DummyRefiner()) {
    config.coarsening.max_allowed_node_weight = 5;
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
  HyperedgeCoarsenerType coarsener;
  std::unique_ptr<IRefiner> refiner;
};

TEST_F(AHyperedgeCoarsener, RemembersMementosOfNodeContractionsDuringOneCoarseningStep) {
  coarsener.coarsen(5);
  ASSERT_THAT(coarsener._contraction_mementos.size(), Eq(2));
  ASSERT_THAT(coarsener._history.back().mementos_begin, Eq(0));
  ASSERT_THAT(coarsener._history.back().mementos_size, Eq(2));
}

TEST_F(AHyperedgeCoarsener, DoesNotEnqueueHyperedgesThatWouldViolateThresholdNodeWeight) {
  config.coarsening.max_allowed_node_weight = 3;
  coarsener.rateAllHyperedges();
  ASSERT_THAT(coarsener._pq.contains(1), Eq(false));
}

TEST_F(AHyperedgeCoarsener, RemovesHyperedgesThatWouldViolateThresholdNodeWeightFromPQonUpdate) {
  config.coarsening.max_allowed_node_weight = 4;
  coarsener.coarsen(6);
  ASSERT_THAT(coarsener._pq.contains(1), Eq(false));
  ASSERT_THAT(coarsener._pq.contains(3), Eq(false));
  ASSERT_THAT(coarsener._pq.contains(0), Eq(true));
}

TEST(HyperedgeCoarsener, RemoveNestedHyperedgesAsPartOfTheContractionRoutine) {
  Hypergraph hypergraph(5, 4, HyperedgeIndexVector { 0, 3, 7, 9,  /*sentinel*/ 11 },
                        HyperedgeVector { 0, 1, 2, 0, 1, 2, 3, 2, 3, 3, 4 });
  hypergraph.setEdgeWeight(1, 5);
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}

TEST(HyperedgeCoarsener, DeleteRemovedSingleNodeHyperedgesFromPQ) {
  Hypergraph hypergraph(3, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 5 },
                        HyperedgeVector { 0, 1, 0, 1, 2 });
  hypergraph.setEdgeWeight(1, 5);
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(1);

  ASSERT_THAT(coarsener._pq.contains(0), Eq(false));
  ASSERT_THAT(coarsener._pq.contains(1), Eq(false));
}

TEST(HyperedgeCoarsener, DeleteRemovedParallelHyperedgesFromPQ) {
  Hypergraph hypergraph(3, 3, HyperedgeIndexVector { 0, 2, 4,  /*sentinel*/ 6 },
                        HyperedgeVector { 0, 1, 0, 2, 1, 2 });
  hypergraph.setEdgeWeight(2, 5);
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(false));
  ASSERT_THAT(coarsener._pq.contains(1), Eq(false));
}

TEST_F(AHyperedgeCoarsener, UpdatesRatingsOfIncidentHyperedgesOfRepresentativeAfterContraction) {
  config.coarsening.max_allowed_node_weight = 25;
  coarsener.coarsen(5);
  ASSERT_THAT(coarsener._pq.contains(2), Eq(false));
  ASSERT_THAT(coarsener._pq.getKey(0), DoubleEq(1.0));
  ASSERT_THAT(coarsener._pq.getKey(1), DoubleEq(EdgeWeightDivMultPinWeight::rate(1, *hypergraph, 25).value));
  ASSERT_THAT(coarsener._pq.getKey(3), DoubleEq(EdgeWeightDivMultPinWeight::rate(3, *hypergraph, 25).value));
  coarsener.coarsen(4);
  ASSERT_THAT(coarsener._pq.contains(0), Eq(false));
  ASSERT_THAT(coarsener._pq.getKey(1), DoubleEq(EdgeWeightDivMultPinWeight::rate(1, *hypergraph, 25).value));
  ASSERT_THAT(coarsener._pq.getKey(3), DoubleEq(EdgeWeightDivMultPinWeight::rate(3, *hypergraph, 25).value));
}

TEST(HyperedgeCoarsener, RestoreParallelHyperedgesDuringUncontraction) {
  Hypergraph hypergraph(3, 3, HyperedgeIndexVector { 0, 2, 4,  /*sentinel*/ 6 },
                        HyperedgeVector { 0, 1, 0, 2, 1, 2 });
  hypergraph.setEdgeWeight(2, 5);
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);
  std::unique_ptr<IRefiner> refiner(new DummyRefiner());

  coarsener.coarsen(2);
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.initializeNumCutHyperedges();
  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(true));
}

TEST(HyperedgeCoasener, RestoreSingleNodeHyperedgesDuringUncontraction) {
  Hypergraph hypergraph(3, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 5 },
                        HyperedgeVector { 0, 1, 0, 1, 2 });
  hypergraph.setEdgeWeight(1, 5);
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);
  std::unique_ptr<IRefiner> refiner(new DummyRefiner());

  coarsener.coarsen(1);
  hypergraph.setNodePart(0, 0);
  hypergraph.initializeNumCutHyperedges();
  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(true));
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(true));
}

TEST_F(AHyperedgeCoarsener, FullyRestoresHypergraphDuringUncontraction) {
  Hypergraph input_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 });
  config.coarsening.max_allowed_node_weight = 10;
  std::unique_ptr<IRefiner> refiner(new DummyRefiner());

  coarsener.coarsen(1);
  hypergraph->setNodePart(0, 0);
  hypergraph->initializeNumCutHyperedges();
#ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  coarsener.uncoarsen(*refiner);

  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(*hypergraph, input_hypergraph), Eq(true));
}

TEST(HyperedgeCoarsener, AddRepresentativeOnlyOnceToRefinementNodes) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 3 },
                        HyperedgeVector { 0, 1, 2 });
  Configuration config;
  HyperedgeCoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);
  std::vector<HypernodeID> refinement_nodes = { 42, 42, 42 };
  size_t num_refinement_nodes = 0;
  refinement_nodes.reserve(hypergraph.initialNumNodes());
  config.coarsening.max_allowed_node_weight = 5;

  coarsener.coarsen(1);
  hypergraph.setNodePart(0, 0);
  hypergraph.initializeNumCutHyperedges();
  coarsener.restoreSingleNodeHyperedges();

  coarsener.performUncontraction(coarsener._history.back(), refinement_nodes,
                                 num_refinement_nodes);

  ASSERT_THAT(num_refinement_nodes, Eq(3));
  ASSERT_THAT(refinement_nodes, UnorderedElementsAre(0, 1, 2));
}
}  // namespace partition
