#include "pch.h"
#include <iostream>
#include "../objects.h"

using namespace std;
using namespace Objects;

// Test types
struct A {
	A(int _x, int _y) : x(_x), y(_y) {}
	int x = 0;
	int y = 0;
};

struct B {
	B(double _x, double _y) : x(_x), y(_y) {}
	double x = 0;
	double y = 0;
};

// Define the object store we're using. This one contains 3 types.
using Store = ObjectStorage<A, B, double>;

// Define simpler way to use object handle
template <typename T>
using Handle = OHandle<T, Store>;

// Use a fixture to initialize a store and add some values
class TestFixture : public ::testing::Test {
public:
	Store store;							// The object store
	std::vector<Handle<A>> a_handles;		// Store handles so we can use them
	std::vector<Handle<B>> b_handles;
	std::vector<Handle<double>> double_handles;

	TestFixture() {
		// Create some objects and store handles
		for (int i = 0; i < 100; i++) {
			a_handles.push_back(store.Create<A>(i, i + 1));
			b_handles.push_back(store.Create<B>(i, i + 2.5));
			double_handles.push_back(store.Create<double>(i + 4.6));
		}
	}
};


TEST_F(TestFixture, InitialCapacities) {
	// There should be 100 total slots for each type
	EXPECT_EQ(store.capacity<A>(), 100);
	EXPECT_EQ(store.capacity<B>(), 100);
	EXPECT_EQ(store.capacity<double>(), 100);

	// There should be no free slots
	EXPECT_EQ(store.free_capacity<A>(), 0);
	EXPECT_EQ(store.free_capacity<B>(), 0);
	EXPECT_EQ(store.free_capacity<double>(), 0);	
}

TEST_F(TestFixture, CapacitiesAfterDeletions) {
	// Pop a handle off the A vector list
	a_handles.pop_back();

	// Clean the store to remove unreferenced objects
	store.Clean();

	// There should be 100 total slots for each type
	EXPECT_EQ(store.capacity<A>(), 100);
	EXPECT_EQ(store.capacity<B>(), 100);
	EXPECT_EQ(store.capacity<double>(), 100);

	// There should now be 1 free slot for A, since there are no more references to the popped object
	EXPECT_EQ(store.free_capacity<A>(), 1);
	EXPECT_EQ(store.free_capacity<B>(), 0);
	EXPECT_EQ(store.free_capacity<double>(), 0);
}

TEST_F(TestFixture, NumberOfReferences) {
	// There should be just 1 reference for all handles
	EXPECT_EQ(b_handles[7].num_references(), 1);

	// Make a copy of a handle
	Handle<B> copy = b_handles[7];

	// There should now be 2 references for this handle
	EXPECT_EQ(b_handles[7].num_references(), 2);
}