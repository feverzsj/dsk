
**`dsk` is a C++ asynchronous framework supporting:**
- Async Cancellation
- Async Cleanup
- Structured Concurrency

**And a rich collection of modules:**
- [Async I/O](include/dsk/asio/)
- [Async http](include/dsk/http/)
- [Async Curl](include/dsk/curl/)
- [Async GRPC](include/dsk/grpc/)
- [Async Redis client](include/dsk/redis/)
- [Async MySQL client](include/dsk/mysql/)
- [Async PostgreSQL client](include/dsk/pq/)
- [Sqlite](include/dsk/sqlite3/)
- [Json centric serialization](include/dsk/jser/) (json, msgpack)

**Supported compiler and platforms:**
- Clang (20.1.6 or later) on windows and linux.

**Exapmles:** [test/](test/), [example/](example/)


# Basics

<!--TOC-->
  - [`task/generator<T, E>`: the `dsk` flavored C++ coroutine](#taskgeneratort-e-the-dsk-flavored-c-coroutine)
  - [Async cancellation](#async-cancellation)
  - [Async cleanup](#async-cleanup)
  - [`_scheduler_`](#_scheduler_)
  - [`_io_scheduler_`](#_io_scheduler_)
  - [`_resumer_`](#_resumer_)
  - [`_continuation_`](#_continuation_)
  - [`_async_ctx_`](#_async_ctx_)
  - [`_async_op_`](#_async_op_)
  - [`resume_on(_scheduler_or_resumer_ sr, _async_op_ op)`](#resume_on_scheduler_or_resumer_-sr-_async_op_-op)
  - [`resume_on(_scheduler_or_resumer_ sr)`](#resume_on_scheduler_or_resumer_-sr)
  - [`start_on(_scheduler_or_resumer_ sr, _async_op_ op)`](#start_on_scheduler_or_resumer_-sr-_async_op_-op)
  - [`run_on(_scheduler_or_resumer_ sr, _async_op_ op)`](#run_on_scheduler_or_resumer_-sr-_async_op_-op)
  - [`solely_run_on(_scheduler_or_resumer_ sr, _async_op_ op)`](#solely_run_on_scheduler_or_resumer_-sr-_async_op_-op)
  - [`sync_wait(_async_op_, ctxArgs...)`](#sync_wait_async_op_-ctxargs...)
  - [`until_all_xxx(_async_op_...)`](#until_all_xxx_async_op_...)
  - [`until_first_xxx(_async_op_...)`](#until_first_xxx_async_op_...)
  - [`async_op_group`](#async_op_group)
  - [`res_pool<T, Creator, Recycler>`](#res_poolt-creator-recycler)
  - [`res_pool_map<Key, T, Creator, Recycler, AutoAddPool>`](#res_pool_mapkey-t-creator-recycler-autoaddpool)
  - [`res_queue<T>`](#res_queuet)
<!--/TOC-->


## `task/generator<T, E>`: the `dsk` flavored C++ coroutine

`task` represents a lazy coroutine (the execution of task does not start until it is awaited). The result of `task<T, E>` is `expected<T, E>`, where `T` is the value type, and `E` is the error type. `expected` is like `std::expected`, but also supports `void` or reference `T`.

```C++
task<T, E> sub_task(auto... args)
{    
    if(something_went_wrong)
    {
        // Construct the result as expected<T, E>(unexpect, e)
        // and return from coroutine.
        DSK_THROW(e);
    }

    try
    {
        expression_may_throw;
    }
    catch(...)
    {
    	// C++ exception must be handled inside coroutine.
    	// They cannot pass through coroutine boundary,
    	// otherwise program will be aborted immediately.
    }

    // Construct the result as expected<T, E>(expect, v)
    // and return from coroutine.
    DSK_RETURN(v);

    // To return local variable or parameter that should be moved:
    DSK_RETURN(std::move(v));
    DSK_RETURN_MOVED(v);

    // For void T, must call DSK_RETURN() at the end of coroutine.
    DSK_RETURN();
}

// By default T = void, E = error_code.
task<> main_task()
{
    // Initiate and wait for task to complete,
    // then return the result of task.
    expected<T, E> r = DSK_WAIT sub_task(args...);

    T v = DSK_TRY sub_task(args...);
    // Same as:
    // expected<T, E> r = DSK_WAIT sub_task(args...);
    // 
    // if(! r.has_value())
    // {
    //     DSK_THROW(std::move(r).error());
    // }
    // 
    // T v = std::move(r).value();
    // 
    // or:
    // expected<T, E> r = DSK_WAIT sub_task(args...);
    // T v = DSK_TRY_SYNC r;

    DSK_RETURN();
}
```

`generator` is like `task`, but can yield value more than once. The result type of yield is `expected<optional<T>, E>`.

```C++
generator<T, E> gen_vals(auto... args)
{
    // Construct the yielded value as expected<optional<T>, E>(expect, v1),
    // pause the coroutine and return to caller.
    DSK_YIELD v1;
    DSK_YIELD v2;

    while(more_vals)
    {
        if(something_went_wrong)
        {
            // Construct the result as expected<optional<T>, E>(unexpect, e)
            // and return from coroutine.
            DSK_THROW(e);
        }

        DSK_YIELD v;
    }

    DSK_RETURN();
}

task<> handle_vals(auto... args)
{
    auto vals = gen_vals(args...);

    // Wait until the generator yielded
    expected<optional<T>, E> r1 = DSK_WAIT vals.next();

    optional<T> v2 = DSK_TRY vals.next();

    while(auto v = DSK_TRY vals.next())
    {
        DSK_TRY handle_val(v);
    }

    DSK_RETURN();
}
```

## Async cancellation

Async cancellation is built on `std::stop_source`. `std::stop_source` is passed to `_asyn_op_` via `_async_ctx_`, but it's disabled (as if constructed via `std::std::nostopstate`) by default. User must explicitly pass in an `_async_ctx_` with a valid `std::stop_source` to enable it.

When a cancellable `_asyn_op_` is initiated, the stop callback is only added when `std::stop_source::stop_possible()` returns true. The cancellation is thread-safe as long as the stop callback is thread-safe.

When an `_asyn_op_` is cancelled, the result will represent some error.


```C++
task<> some_task()
{
    // Forward error to parent, including cancellation.
    DSK_TRY cancellable_op;

    // or manually handle it.
    auto r = DSK_WAIT cancellable_op;

    if(has_err(r))
    {
        if(get_err(r) == op_is_cancelled)
        {
            handle_cancelled_op();
        }

        DSK_THROW(get_err(r));
    }

    DSK_RETURN();
}
```

User can also deal with `std::stop_source` directly.

```C++
task<> cancellable_task()
{
    auto ss = DSK_WAIT get_stop_source();
    auto st = DSK_WAIT get_stop_token(); // same as: ss.get_token();
    bool sp = DSK_WAIT stop_possible();  // same as: ss.stop_possible();
    bool sr = DSK_WAIT stop_requested(); // same as: ss.stop_requested();
    bool rs = DSK_WAIT request_stop();   // same as: ss.request_stop();

    auto sc = DSK_WAIT create_stop_callback([](){ do_cancel(); });
    // same as: std::stop_callback(ss.get_token(), f);

    for(;;)
    {
        if(DSK_WAIT stop_requested())
        {
            DSK_THROW(errc::operation_canceled);
        }

        continue_process();
    }

    DSK_RETURN();
}
```

To explicitly pass in a `std::stop_source`.

```C++
std::stop_source ss;
sync_wait(root_task(), ss);
```

## Async cleanup

Some `_async_op_`s need to be cleaned up by another `_async_op_` after successful initiation. The cleanup should be executed at the proper time, and the op should not be destroyed until the cleanup is finished.

Cleanup ops cannot be cancelled, and all of them should be finished, no matter the results.

`task` and `generator` support such cleanup mechanism as cleanup scope.

```C++
task<> some_task()
{
    {
        auto scope = DSK_WAIT make_cleanup_scope();

        // Explicitly add cleanup op
        scope.add(asyncCleanupOp);
        DSK_WAIT add_cleanup(asyncCleanupOp); // same as above

        // Implicitly add cleanup op
        auto r = DSK_TRY asyncOpThatAddCleanupOp;

        // Initiate all cleanup ops added to this scope
        // and wait them to finish.
        DSK_TRY scope.until_all_done();
    }

    // macros
    {
        DSK_CLEANUP_SCOPE_BEGIN();

        DSK_CLEANUP_SCOPE_ADD(asyncCleanupOp);

        DSK_CLEANUP_SCOPE_END();
    }

    // nested cleanup scopes
    {
        DSK_CLEANUP_SCOPE_BEGIN();

        DSK_CLEANUP_SCOPE_ADD(asyncCleanupOp1);

        {
            DSK_CLEANUP_SCOPE_BEGIN();

            DSK_CLEANUP_SCOPE_ADD(asyncCleanupOp2);

            // Initiate asyncCleanupOp2 and wait it to finish.
            DSK_CLEANUP_SCOPE_END();
        }

        // Initiate asyncCleanupOp1 and wait it to finish.
        DSK_CLEANUP_SCOPE_END();
    }

    {
        auto scope1 = DSK_WAIT make_cleanup_scope();

        {
            auto scope2 = DSK_WAIT make_cleanup_scope();

            if(something_went_wrong)
            {
                // At exit points: DSK_RETURN/DSK_THROW/DSK_TRY/DSK_TRY_SYNC,
                // cleanup ops will also be executed scope by scope,
                // from the closest to the outermost.
                DSK_THROW(e);
                // 1. Construct the result as expected(unexpect, e).
                // 2. Initiate and wait cleanup ops of scope2 to finish.
                // 3. Initiate and wait cleanup ops of scope1 to finish.
                // If any error raises from cleanup ops,
                // errc::one_or_more_cleanup_ops_failed will overwirte the result.
            }

            DSK_TRY scope2.until_all_done();
            // since this is also a potential exit point, similar thing will happen. On failure:
            // 1. Construct the result as expected(unexpect, errc::one_or_more_cleanup_ops_failed);
            // 2. Initiate and wait cleanup ops of scope1 to finish.
        }

        DSK_TRY scope1.until_all_done();
    }

    // Function scope is the implicit cleanup scope.

    // Add to implicit cleanup scope
    DSK_WAIT add_cleanup(asyncCleanupOp);

    // To end the implicit cleanup scope.
    // DSK_RETURN must be always placed at the end of function, even for void T.
    DSK_RETURN();
}
```

Async cleanup can be used in `generator` just like in `task`, even when the generator doesn't finish.
An `_async_op_` to cleanup `generator`'s cleanup scopes is added to current cleanup scope, when first waited.

```C++
task<> handle_unfinished_gen()
{
    {
        DSK_CLEANUP_SCOPE_BEGIN();

        auto scopedGen = []() -> generator<int>
        {
            DSK_WAIT add_cleanup(genCleanupOp);

            DSK_YIELD 1;
            DSK_YIELD 2;

            DSK_RETURN();
        }();

        auto v = DSK_TRY scopedGen.next();
        assert(v == optional(1));

        // genCleanupOp will be executed here,
        // as the generator is not finished.
        DSK_CLEANUP_SCOPE_END();
    }
}

```

## `_scheduler_`

`_scheduler_.post(nullaryCallable)` schedules the execution of nullary callable.

- `inline_scheduler`: invokes the callable directly.
- thread pool schedulers: schedule callable on thread pool for later execution.
  - `naive_thread_pool`: a single lock round-robin style thread pool.
  - `simple_thread_pool`: a simple work stealing thread pool.
  - `asio_thread_pool`: based on `asio::thread_pool`.
  - `asio_io_thread_pool`: based on `asio::io_context`.
  - `tbb_thread_pool`: explicit `tbb::task_arena` managed thread pool.
  - `tbb_implicit_thread_pool`: use implict global `tbb::task_arena`.
  - `win_thread_pool`: based on Win32 Threadpool.


## `_io_scheduler_`

A `_scheduler_` provides the core I/O functionality for users of the asynchronous I/O objects. `asio_io_thread_pool` is the only `_io_scheduler_`.


## `_resumer_`

Lightweight agent to `_scheduler_`. `_resumer_.post(callable)` is same as `_scheduler_.post(callable)`. `_resumer_`s can be compared for equality. If 2 `_resumer_`s reference the same `_scheduler_`, they are equal.

`_async_op_` typically resumes `_continuation_` using `_resumer_` of `_async_ctx_`.


## `_continuation_`

Any nullary callable used as continuation of `_async_op_`.


## `_async_ctx_`

Lightweight object for accessing `stop_source`, `_resumer_` and async cleanup of the initiation context.

```C++
class async_ctx
{
public:
    // This is an _async_ctx_.
    using is_async_ctx  = void;

    // Get std::stop_source. [optional]
    // If not present, std::stop_souce(std::nostopstate) is used.
    decltype(auto) get_stop_source();

    // Get _resumer_. [optional]
    // If not present, inline_resumer is used.
    decltype(auto) get_resumer();

    // Add cleanup op. [optional]
    // If not present, async cleaup is not supported by this context.
    void add_cleanup(_async_op_ auto&&);
};
```

## `_async_op_`


```C++
class async_op
{
public:
    // This is an _async_op_.
    using is_async_op  = void;

    // Returns if initiate() should be skipped. [optional]
    bool is_immediate();

    // Initiate this op. [optional]
    // parameters:
    //     ctx: the _async_ctx_. [optional]
    //     cont: the _continuation_. [optional]
    // Return type:
    //     void: op is initiated.
    //     bool: true for initiated, false for finished.
    //     std::coroutine_hanle<P>: std::noop_coroutine() for initiated,
    //                              others for completed by resuming the returned coroutine.
    auto initiate([_async_ctx_ auto&& ctx], [_continuation_ auto&& cont]);

    // Returns if the result represents some failure. [optional]
    // Can only be called after the op being finished.
    bool is_failed();

    // Take the result. [optional]
    // Return type can be value, void or lvalue reference.
    // Should be only called once, typically inside continuation.
    // Can only be called ONCE after the op has finished.
    // No other method can be called after take_result().
    decltype(auto) take_result();
};
```

`_async_op_` is similar to C++ awaitable:

- `is_immediate()` -> `await_ready()`
- `initiate()` -> `await_suspend()`
- `take_result()` -> `await_resume()`
 
The key difference are `_async_ctx_` and `_continuation_`.


## `resume_on(_scheduler_or_resumer_ sr, _async_op_ op)`

Returns an `_async_op_` that, once waited, resumes the `_continuation_` of `op` on `sr`.


## `resume_on(_scheduler_or_resumer_ sr)`

Returns an `_async_op_` that, once waited, resumes the current coroutine on `sr`.


## `start_on(_scheduler_or_resumer_ sr, _async_op_ op)`

Returns an `_async_op_` that, once waited, initiates `op` on `sr`.


## `run_on(_scheduler_or_resumer_ sr, _async_op_ op)`

Returns an `_async_op_` that, once waited, initiates `op` on `sr` with an `_async_ctx_` that uses 'sr' as `_resumer_`.


## `solely_run_on(_scheduler_or_resumer_ sr, _async_op_ op)`

Returns an `_async_op_` that, once waited, initiates `op` on `sr` with an `_async_ctx_` that uses 'sr' as `_resumer_`, and also resumes the `_continuation_` of `op` on `sr`.


## `sync_wait(_async_op_, ctxArgs...)`

Initiate the `_async_op_` with `_async_ctx_ = make_async_ctx(ctxArgs...)`, synchronously wait it to finish and return its result.

## `until_all_xxx(_async_op_...)`

- `until_all_done`: wait until all ops to finish.
- `until_all_succeeded`: wait until all ops to finish successfully. If any op failed, unfinished ops are canceled and errc::failed is returned.

The result type is `expected<tupe<result_type_of_ops...>>`.

```C++
task<> some_task()
{
    auto v = DSK_TRY until_all_done(asyncOp0, asyncOp1, ...);

    auto& r0 = std::get<0>(v);
    auto& r1 = std::get<1>(v);

    DSK_RETURN();
}
```

## `until_first_xxx(_async_op_...)`

Wait until first finished op that meets the condition, then cancel unfinished ops. If no op met the condition, return errc::not_found.

- `until_first_done`: wait until first op to finish.
- `until_first_succeeded`: wait until first op to finish successfully.
- `until_first_failed`: wait until first op to finish unsuccessfully.

The result type is `expected<std::varaint<result_type_of_ops...>>` where `varaint.index()` points to the first finished op that meets the condition.

```C++
task<> some_task()
{
    auto v = DSK_TRY until_first_succeeded(asyncOp0, asyncOp1, ...);

    std::visit(
        [&](auto& r){ process_result(r); },
        v);

    DSK_RETURN();
}
```

## `async_op_group`

`async_op_group` tracks and manages `_async_op_`s added to it. It's like `until_all_done`, but can handle undetermined number of ops.

```C++
task<> some_task()
{
    {
        DSK_CLEANUP_SCOPE_BEGIN();

        // grp.until_all_done() is also added
        // to current cleanup scope.
        auto grp = DSK_WAIT make_async_op_group();

        while(has_more_async_op)
        {
            // Add and initiate asyncOp.
            // grp takes ownership of asyncOp.
            grp.add(std::move(asyncOp));
        }

        // Initiate grp.until_all_done() and
        // wait for all asyncOps added to grp to finish.
        DSK_CLEANUP_SCOPE_END();
    }

    // grp.until_all_done() is added
    // to implicit function cleanup scope.
    auto grp = make_async_op_group();

    // You can call grp.until_all_done() explicitly.
    // If any of the added op fails,
    // errc::one_or_more_ops_failed will be returned.
    DSK_TRY grp.until_all_done();

    DSK_RETURN();
}
```

## `res_pool<T, Creator, Recycler>`

`res_pool` manages and limits concurrent access to a number of resource `T`. `T` can be value type or `void`.

```C++
task<> some_task()
{
    // Resource creator directly emplaces resource into the pool.
    // It is called under a lock, when there is no available resource
    // and the total resource count hasn't reached the capacity.
    // Use res_pool::create_all() to precreate all resource.
    auto creator = [i = 0](auto emplace) mutable { emplace(++i); };

    // Construct a pool of int with a capacity of 2.
    res_pool<int, decltype(creator)> pool(2, creator);

    // Try acquire resource without waiting.
    // The result type is expected<res_ref>.
    auto v1 = DSK_TRY_SYNC pool.try_acquire();
    auto v2 = DSK_TRY_SYNC pool.try_acquire();

    // res_ref is a proxy to resource.
    // Use res_ref::get/operator*/operator-> to access resource.
    assert(*v1.get() == 1);
    assert(*v2 == 2);

    // When no resource available,
    // errc::resource_unavailable will be returned.
    auto r3 = pool.try_acquire();
    assert(is_err(r3, errc::resource_unavailable));

    // Call res_ref::recycle() to put resource back in
    // the pool, which will be also called by its destructor.
    v2.recycle();

    auto grp = DSK_WAIT make_async_op_group();
    grp.add([&]()->task<>
    {
        auto v2 = DSK_TRY_SYNC pool.try_acquire();
        assert(*v2 == 2);
        // Hold the resource for a while.
        DSK_TRY wait_for(std::chrono::milliseconds(100));
        DSK_RETURN();
    }());

    // Wait until resource is available.
    v2 = DSK_TRY pool.acquire();
    assert(*v2 == 2);

    DSK_RETURN();
}
```

## `res_pool_map<Key, T, Creator, Recycler, AutoAddPool>`

`res_pool_map` is basically a map of `res_pool`. You can acquire resource from pools via keys.

```C++
task<> some_task()
{
    std::atomic<int> i;
    auto defaultCreator = [&i](auto emplace) mutable { emplace(++i); };

    // Each pool will be constructed with a capacity of 2 and defaultCreator.
    res_pool_map<int, int, decltype(defaultCreator)> poolMap(2, defaultCreator);

    // Try acquire resource from pool via key without waiting.
    // If AutoAddPool and the pool doesn't exists,
    // a new pool will be added for the key.
    auto v1 = DSK_TRY_SYNC poolMap.try_acquire(1);
    auto v2 = DSK_TRY_SYNC poolMap.try_acquire(2);
    auto r3 = poolMap.try_acquire(2);

    assert(*v1.get() == 1);
    assert(*v2 == 2);
    assert(is_err(r3, errc::resource_unavailable));

    v2.recycle();

    auto grp = DSK_WAIT make_async_op_group();
    grp.add([&]()->task<>
    {
        auto v2 = DSK_TRY_SYNC poolMap.try_acquire(2);
        assert(*v2 == 2);
        DSK_TRY wait_for(std::chrono::milliseconds(100));
        DSK_RETURN();
    }());

    v2 = DSK_TRY poolMap.acquire(2);
    assert(*v2 == 2);

    // Add and use pool manually

    auto[pool3, isNew] = poolMap.try_emplace_pool(3);
    assert(isNew);

    auto v3 = DSK_TRY_SYNC pool3->try_acquire();
    assert(*v3 == 3);

    DSK_RETURN();
}
```

## `res_queue<T>`

`res_queue` is a bounded queue that supports async enqueue and dequeue.

```C++
task<> some_task()
{
    // Construct a queue of int with a capacity of 2.
    res_queue<int> queue(2);

    DSK_TRY until_all_succeeded
    (
        // Give coroutine's _async_ctx_ a _resumer_.
        // The default inline_resumer is mostly not suitable for res_queue.
        run_on(DSK_DEFAULT_IO_SCHEDULER, [&]() -> task<>
        {
            // Try enqueue without waiting.
            DSK_TRY_SYNC queue.try_enqueue(1);
            DSK_TRY_SYNC queue.try_enqueue(2);
            auto r = queue.try_enqueue(3);
            CHECK(is_err(r, errc::out_of_capacity));

            DSK_TRY wait_for(std::chrono::milliseconds(1000));
            
            // Wait until there is free capacity.
            DSK_TRY queue.enqueue(3);
            DSK_TRY queue.enqueue(4);
            DSK_TRY queue.enqueue(5);
            DSK_TRY queue.enqueue(6);

            DSK_TRY wait_for(std::chrono::milliseconds(1000));
            
            // Mark end of queue.
            bool isFirst = queue.mark_end();
            CHECK(isFirst);

            // No new item can be enqueued of end of queue.
            auto r2 = DSK_WAIT queue.enqueue(66);
            CHECK(is_err(r2, errc::end_reached));

            DSK_RETURN();
        }()),
        run_on(DSK_DEFAULT_IO_SCHEDULER, [&]() -> task<>
        {
            DSK_TRY wait_for(std::chrono::milliseconds(500));
           
            // Wait until resource is available.
            int v1 = DSK_TRY queue.dequeue();
            int v2 = DSK_TRY queue.dequeue();
            CHECK(v1 == 1);
            CHECK(v2 == 2);

            // Try dequeue without waiting.
            // The result type is expected<T>.
            auto r = queue.try_dequeue();
            CHECK(is_err(r, errc::resource_unavailable));

            // Wait until there is new item.
            int v3 = DSK_TRY queue.dequeue();
            int v4 = DSK_TRY queue.dequeue();
            CHECK(v3 == 3);
            CHECK(v4 == 4);

            DSK_TRY wait_for(std::chrono::milliseconds(500));
            int v5 = DSK_TRY queue.dequeue();
            int v6 = DSK_TRY queue.dequeue();
            CHECK(v5 == 5);
            CHECK(v6 == 6);

            // End of queue is reached.
            auto r2 = DSK_WAIT queue.dequeue();
            CHECK(is_err(r2, errc::end_reached));

            DSK_RETURN();
        }())
    ));

    DSK_RETURN();
}
```
