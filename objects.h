#pragma once
#pragma once

#include <tuple>
#include <vector>
#include <variant>
#include <algorithm>
#include <concepts>
#include <functional>
#include "typelist.h"

namespace Objects {

	// Holes form a linked list within the object store
	struct Hole {
		uint32_t next_hole = 0;
	};

	// Using a variant that can be either an object or a hole
	template<typename T>
	struct ObjectOrHole {
		typedef T type;
		uint16_t references = 0;		// The number of live handles
		std::variant<Hole, T> var;		// The variant

		// If a hole, get the hole
		Hole* gHole() {
			return std::get_if<Hole>(&var);
		}

		// Get the next hole, assuming this is a hole. Only used internally.
		uint32_t gNextHole() {
			// Get the hole object from variant
			auto* hole = std::get_if<Hole>(&var);
			// Return the next hole index
			return hole->next_hole;
		}

		// Test whether this is an object
		bool IsObject() { return std::holds_alternative<T>(var); }

		// Get the raw pointer to the object, or null if a hole
		T* gObject() {
			return std::get_if<T>(&var);
		}

		ObjectOrHole() {}

		// An ObjectOrHole cannot be copied
		ObjectOrHole(const ObjectOrHole& other) = delete;
		ObjectOrHole& operator=(const ObjectOrHole& other) = delete;
		ObjectOrHole(ObjectOrHole&& other) = default;
		ObjectOrHole& operator=(ObjectOrHole&& other) = default;
	};

	// Helper for a vector of ObjectOrHoles
	template <typename T>
	using ObjectOrHoleVector = std::vector<ObjectOrHole<T>>;

	// The base Handle type
	template <typename T, typename ObjectStorageType>
	class OHandle {
	private:
		ObjectStorageType* objectStore = nullptr;				// Handles store a pointer to the object store
	public:
		uint32_t index = 0;										// The index of the object in the object store
		typedef T type;
		// Default constructor leaves the object store as nullptr
		// Represents an empty handle
		OHandle() : index(0) {}

		// This constructor sets the index and the object store in which the underlying object was created.
		// Registers a reference upon creation
		OHandle(uint32_t i, ObjectStorageType* os) : index(i), objectStore(os) {
			objectStore->RegisterReference(*this);
		}

		// Destructor deregisters a reference as long as handle is not empty
		~OHandle() {
			if (index > 0) objectStore->DeregisterReference(*this);
		}

		// Copy constructor
		OHandle(const OHandle& rhs) {
			// Copy the index and the objectStore
			index = rhs.index;
			objectStore = rhs.objectStore;

			// Register a reference if not empty
			if (index > 0) objectStore->RegisterReference(*this);
		}

		// Copy assignment constructor
		OHandle& operator=(const OHandle& rhs) {
			// Deregister a reference if the original handle is valid
			if (index > 0) objectStore->DeregisterReference(*this);

			// Set the index and objectStore
			index = rhs.index;
			objectStore = rhs.objectStore;

			// Register a reference if not empty
			if (index > 0) objectStore->RegisterReference(*this);

			// Return handle
			return *this;
		}

		// Move operator
		OHandle(OHandle&& rhs) noexcept {
			// Copy the index and the objectStore
			index = rhs.index;
			objectStore = rhs.objectStore;

			// Register a reference if not empty
			if (index > 0) objectStore->RegisterReference(*this);
		}

		// Move assignement operator
		OHandle& operator=(const OHandle&& rhs) {
			// Deregister a reference if the original handle is valid
			if (index > 0) objectStore->DeregisterReference(*this);

			// Set the index and objectStore
			index = rhs.index;
			objectStore = rhs.objectStore;

			// Register a reference if not empty
			if (index > 0) objectStore->RegisterReference(*this);

			// Return handle
			return *this;
		}

		// Get number of references to this handle
		size_t num_references() {
			if (index > 0) {
				return objectStore->GetNumReferences(*this);
			}
			else return 0;
		}

		// Direct dereference of a handle
		T* operator->() {
			return ((index > 0) ? objectStore->Get(*this) : nullptr);
		}

		// Direct dereference of a handle
		T* operator*() {
			return ((index > 0) ? objectStore->Get(*this) : nullptr);
		}

		// Checks if indices are equal
		bool operator==(const OHandle& rhs) const {
			return (index == rhs.index);
		}

		// Comparison of indices
		bool operator<(const OHandle& rhs) const {
			return (index < rhs.index);
		}
	};

	/////////////////////////////////////////////////////////
	// Stores a vector for each type of object
	/////////////////////////////////////////////////////////
	template <typename... Ts>
	class ObjectStorage {
	private:
		using OS = ObjectStorage<Ts...>;
		using types = typelist<Ts...>;
		std::tuple < ObjectOrHoleVector<Ts>...> data;
		bool _hasChanged = false;
	public:
		// Each vector has 1 element initially, a hole at index 0
		ObjectStorage() {
			std::apply([](auto&&... vectors) {(vectors.resize(1), ...); }, data);
		}

		// The 0 element is not used to store data, only to point to first real hole. We subtract 1 from vector length to get capacity.
		template <typename T>
		size_t capacity() {
			return std::get<ObjectOrHoleVector<T>>(data).size() - 1;
		}

		// This is the number of holes, excluding first hole, which must always be present.
		template <typename T>
		uint32_t free_capacity() {
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// Start at zero
			uint32_t index = 0;
			uint32_t count = 0;

			// While there is a next hole that doesn't point back to beginning
			while (uint32_t next = vec[index].gNextHole()) {
				count++;
				index = next;
			}
			return count;
		}

		// Create a new object and return a handle
		template <typename T, typename... Args>
		OHandle<T, OS> Create(Args&&... args) {
			_hasChanged = true;
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// The 0 element points to an available hole, if any. Get the index of next hole
			const uint32_t next_hole = vec.front().gNextHole();

			// If the next hole points back to 0 element, there are no open holes. Must push back vector.
			if (!next_hole) {
				// Create a new object variant
				ObjectOrHole<T> new_variant;

				// Create a new object and set
				new_variant.var = T(std::forward<Args>(args)...);

				// Push back into vec
				vec.push_back(std::move(new_variant));

				// Create and return the handle for the object
				return OHandle<T, OS>(vec.size() - 1, this);
			}
			// If there is a hole available, create the object in the hole location
			else {
				// Get the hole pointed to by next_hole
				const uint32_t next_next_hole = vec[next_hole].gNextHole();

				// Set the 0 element next hole to this value
				vec.front().gHole()->next_hole = next_next_hole;

				// Create new object and store in place on next_hole
				vec[next_hole].var = T(std::forward<Args>(args)...);

				// Set the number of references to 1
				vec[next_hole].references = 1;

				// Create and return handle
				return OHandle<T, OS>(next_hole, this);
			}
		}

		// Get an object by handle
		template <typename T>
		T* Get(OHandle<T, ObjectStorage<Ts...>>& handle) {
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// Return pointer
			return vec[handle.index].gObject();
		}

		// Remove any objects that have no live references. Do this periodically.
		void Clean() {
			std::apply([this](auto&&... vectors) {
				(std::for_each(vectors.begin(), vectors.end(), [&, i = 0](auto& entry) mutable {
					if (entry.IsObject() && entry.references == 0) {
						// Change the type to hole, with next hole pointing to what the 0 element points to
						entry.var = Hole({ vectors.front().gNextHole() });

						// Set next hole for the 0 element to this location
						vectors.front().gHole()->next_hole = i;
					}
					++i;
				}), ...);
			}, data);
		}

		// Get the number of references for handle
		template <typename T>
		uint32_t GetNumReferences(OHandle<T, OS>& handle) {
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// Return number of outstanding references
			return vec[handle.index].references;
		}

		// Register a reference for a handle
		template <typename T>
		void RegisterReference(OHandle<T, OS>& handle) {
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// Add one to reference count
			vec[handle.index].references++;
		}

		// Deregister a reference for a handle
		template <typename T>
		void DeregisterReference(OHandle<T, OS>& handle) {
			// Pull the correct vector based on type
			auto& vec = std::get<ObjectOrHoleVector<T>>(data);

			// Subtract one to reference count
			vec[handle.index].references--;
		}
	};
}
