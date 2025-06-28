
Async I/O module is built on `asio`. Completion tokens are provied to adapt `asio` asynchronous operation to `_async_op_`. It also offers convenient wrappers for common `asio` classes and functions.


## `use_async_op / use_async_op_for<T>`

Completion token that adapts `asio` asynchronous operation to `_async_op_`.

Result type is `expected<T>`, where `T` is the second paramter type of the asynchronous operation's completion signature.

`use_async_op_for<T>` is like `use_async_op`, but forces the type to be `T`.

Async cancellation is supported via `asio`'s cancellation signal/slot.

```C++
task<> some_task()
{
    size_t n = DSK_TRY asio::async_read(s, b, use_async_op);
    DSK_RETURN();
}
```


## Timer and timed op

```C++
task<> some_task()
{
    DSK_TRY wait_for(std::chrono::seconds(6));
    // at least 6 seconds passed here.

    // same as above
    DSK_TRY wait_until(steady_timer::now() + std::chrono::seconds(6));

    // timed op
    auto r = DSK_WAIT wait_for(std::chrono::seconds(6), some_async_op());

    if(is_err(r, errc::timeout))
    {
        // Timeout. The op is canceled.
    }

    DSK_RETURN();
}
```


## send_file(socket, stream_file, send_file_options = {offset = 0, count = -1})

Sends `count` bytes started at `offset` from file to socket. If `count` is -1, all remaining bytes started at `offset` are sent.

The resutl type is `expected<size_t>` for sent bytes.

Currently, zero copy sending is only supported on Windows via `TransmitFile`.


## recv_file(socket, stream_file, recv_file_options = {offset = 0, count = -1})

Receives `count` bytes from socket to file started at `offset`. If `count` is -1, all remaining bytes until eof are received.

The resutl type is `expected<size_t>` for received bytes.
