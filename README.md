# Contiguous Object Pool

This is an object pool implementation that I built for previous project. There are a few key concepts:

- **ObjectStorage** - This is a tuple of vectors, with each vector storing a particular type of object. Objects are stored in contiguous memory, with the index into the array serving as the id.
- **Handle** - A reference to an object. Returned when an object is created, and used to access the object.
- **Hole** - A slot in an ObjectStorage vector can be either an object or a hole. The holes form a linked list. When an object is created, it will fill a hole if available.

You can create ObjectStorage with three types, `A`, `B`, and `double`, like this:

```cpp
	auto store = ObjectStorage<A,B, double>();
```

Create a new object and pass off any arguments for the constructor. This will return a handle.

```cpp
	auto handle = store.Create<A>(arg1, arg2);
```

Handles can be used like pointers. Because handles are reference-counted, if you have a handle, it's guaranteed that the object exists.  

```cpp
	int x = handle->x;
```

When there are no live handles for an object, the reference count for that object will be zero. Call the `Clean` function to convert these into holes:

```cpp
	store.Clean();
```

Copy and move constructors for handles keep the reference count correct. Objects themselves are not copyable and are never moved, ensuring that the index into an array always points to the same object.

To use the object pool, include `objects.h` and `typelist.h` in your project.






