/*
 * greedy_hypergraph_growing_test.cc
 *
 *  Created on: 21.05.2015
 *      Author: theuer
 */

#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/test/greedy_hypergraph_growing_TestFixtures.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "lib/datastructure/FastResetBitVector.h"


using ::testing::Eq;
using ::testing::Test;

using datastructure::NoDataBinaryMaxHeap;
using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;


using Gain = HyperedgeWeight;


namespace partition {

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	base->insertNodeIntoPQ(2,0,-1,true);
	base->insertNodeIntoPQ(4,0,-1,true);
	base->insertNodeIntoPQ(6,0,-1,true);
	ASSERT_EQ(base->size(),3);
	ASSERT_TRUE(base->maxFromPartition(0) == 2 && base->maxKeyFromPartition(0) == 0);
}



TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectGainValueAfterUpdatePriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}
	hypergraph.initializeNumCutHyperedges();
	base->insertNodeIntoPQ(0,1,-1,true);
	ASSERT_EQ(base->maxFromPartition(1),0);
	ASSERT_EQ(base->maxKeyFromPartition(1),2);
	hypergraph.changeNodePart(2,1,0);
	base->insertNodeIntoPQ(0,1,-1,true);
	ASSERT_EQ(base->maxFromPartition(1),0);
	ASSERT_EQ(base->maxKeyFromPartition(1),0);
}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdate) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}
	hypergraph.initializeNumCutHyperedges();
	base->insertNodeIntoPQ(0,1,-1,false);
	base->insertNodeIntoPQ(2,0,-1,false);
	base->insertNodeIntoPQ(4,0,-1,false);
	base->insertNodeIntoPQ(6,0,-1,false);

	hypergraph.changeNodePart(2,1,0);
	base->insertAndUpdateNodesAfterMove(2,0,1,false);

	hypergraph.changeNodePart(4,1,0);
	base->insertAndUpdateNodesAfterMove(4,0,1,false);

	ASSERT_TRUE(base->maxFromPartition(0) == 6 && base->maxKeyFromPartition(0) == 0);
	ASSERT_EQ(base->size(), 2);

}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, GetCorrectMaxGainHypernodeAndPartition) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	std::vector<PartitionID> parts {0, 1};
	std::vector<bool> enable {true, true};
	config.initial_partitioning.unassigned_part = 0;
	config.initial_partitioning.upper_allowed_partition_weight[0] = 7;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 7;

	base->insertNodeIntoPQ(0,1,-1,false);
	base->insertNodeIntoPQ(2,0,-1,false);
	base->insertNodeIntoPQ(4,0,-1,false);
	base->insertNodeIntoPQ(6,0,-1,false);
	HypernodeID best_node = 1000;
	PartitionID best_part = -1;
	Gain best_gain = -1;
	base->getGlobalMaxGainMove(best_node, best_part, best_gain, enable, parts, -1);
	ASSERT_EQ(best_node,0);
	ASSERT_EQ(best_part, 1);
	ASSERT_EQ(best_gain,2);
}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, DeletesAssignedHypernodesFromPriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	config.initial_partitioning.unassigned_part = 0;
	config.initial_partitioning.upper_allowed_partition_weight[0] = 7;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 7;

	base->insertNodeIntoPQ(0,1,-1,false);
	base->insertNodeIntoPQ(2,0,-1,false);
	base->insertNodeIntoPQ(4,0,-1,false);
	base->insertNodeIntoPQ(6,0,-1,false);
	base->deleteNodeInAllBucketQueues(0);
	ASSERT_EQ(base->size(),3);
}



TEST_F(AGreedySequentialBisectionTest, ExpectedGreedySequentialBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,5};
	std::vector<HypernodeID> partition_one {3,4,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}

TEST_F(AGreedyGlobalBisectionTest, ExpectedGreedyGlobalBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,5};
	std::vector<HypernodeID> partition_one {3,4,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}

TEST_F(AGreedyRoundRobinBisectionTest, ExpectedGreedyRoundRobinBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,3,4};
	std::vector<HypernodeID> partition_one {2,5,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}


TEST_F(AKWayGreedySequentialTest, HasValidImbalance) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedySequentialTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedySequentialTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AKWayGreedyGlobalTest, HasValidImbalance) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedyGlobalTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedyGlobalTest, LeavesNoHypernodeUnassigned) {
	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AKWayGreedyRoundRobinTest, HasValidImbalance) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedyRoundRobinTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedyRoundRobinTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AGreedyRecursiveBisectionTest, HasValidImbalance) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k), config.partition.epsilon);

}

TEST_F(AGreedyRecursiveBisectionTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if (heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),
				(lower_bound_factor / 100.0) * heaviest_part);
	}

}

TEST_F(AGreedyRecursiveBisectionTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 4;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for (HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn), -1);
	}
}

};



