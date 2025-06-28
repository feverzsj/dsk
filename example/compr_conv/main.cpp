#ifndef NDEBUG
#define DSK_USE_STD_UNORDERED
#endif
#include <dsk/util/debug.hpp>
#include <dsk/util/deque.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/string.hpp>
#include <dsk/util/perf_counter.hpp>
#include <dsk/compr/zlib.hpp>
#include <dsk/compr/zstd.hpp>
#include <dsk/asio/file.hpp>
#include <dsk/res_queue.hpp>
#include <dsk/sync_wait.hpp>
#include <dsk/start_on.hpp>
#include <dsk/until.hpp>
#include <dsk/task.hpp>
#include <thread>


using namespace dsk;


constexpr size_t fileReadBatchSize = 1*1024*1024;
res_queue<string> decompressQueue{6};
res_queue<string> compressQueue{26};
res_queue<string> writeFileQueue{6};


auto gen_queue_report(char const* name, auto& q)
{
    return cat_as_str("\n      ", name, ": enqueueWait=", with_fmt<'f', 1>(100 * q.stats().enqueue_wait_rate()), "%"
                                        ", dequeueWait=", with_fmt<'f', 1>(100 * q.stats().dequeue_wait_rate()), "%\n");
}


task<> read_file(stream_file file)
{
    perf_counter<B_, MiB> cnter("File Read", "MB");
    periodic_reporter rpt;

    char buf[fileReadBatchSize];

    for(;;)
    {
        auto cnterScope = cnter.begin_count_scope();
        auto r = DSK_WAIT file.read_some(buf);

        if(has_err(r))
        {
            if(get_err(r) == asio::error::eof)
            {
                stdout_report("File end reached.\n");
                decompressQueue.mark_end();
                break;
            }

            DSK_THROW(get_err(r));
        }

        size_t n = get_val(r);
        cnterScope.add_and_end(B_(n));
        DSK_TRY decompressQueue.enqueue(buf, n);

        rpt.report([&](){ return gen_queue_report("fileRead->decompress", decompressQueue); });
    }

    DSK_RETURN();
}

task<> decompress()
{
    perf_counter<B_, MiB> cnter("Decompress", "MB");
    periodic_reporter rpt;

    zlib_decompressor decompr;
    DSK_TRY_SYNC decompr.reinit();

    string buf;

    for(;;)
    {
        auto r = DSK_WAIT decompressQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                if(buf.size())
                {
                    DSK_TRY compressQueue.enqueue(mut_move(buf));
                }

                compressQueue.mark_end();
                break;
            }

            DSK_THROW(get_err(r));
        }
        
        auto& in = get_val(r);

        auto cnterScope = cnter.begin_count_scope();
        auto u = DSK_TRY_SYNC decompr.append_multi(buf, in);
        cnterScope.add_and_end(B_(u.nOut));

        if(buf.size())
        {
            DSK_TRY compressQueue.enqueue(mut_move(buf));
        }

        rpt.report([&](){ return gen_queue_report("decompress->compress", compressQueue); });
    }

    DSK_RETURN();
}

task<> compress()
{
    perf_counter<B_, MiB> cnter("compress", "MB");
    periodic_reporter rpt;

    zstd_compressor compr;
    DSK_TRY_SYNC compr.set(ZSTD_c_compressionLevel, 6,
                           ZSTD_c_nbWorkers, std::thread::hardware_concurrency() - 1);
    string buf;

    for(;;)
    {
        auto r = DSK_WAIT compressQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                DSK_TRY_SYNC compr.finish(buf);
                DSK_TRY writeFileQueue.enqueue(mut_move(buf));
                writeFileQueue.mark_end();
                break;
            }

            DSK_THROW(get_err(r));
        }

        auto& in = get_val(r);

        auto cnterScope = cnter.begin_count_scope();
        DSK_TRY_SYNC compr.append(buf, in);
        cnterScope.add_and_end(B_(in.size()));

        if(buf.size())
        {
            DSK_TRY writeFileQueue.enqueue(mut_move(buf));
        }

        rpt.report([&](){ return gen_queue_report("compress->fileWrite", writeFileQueue); });
    }

    DSK_RETURN();
}

task<> write_file(stream_file file)
{
    perf_counter<B_, MiB> cnter("File Write", "MB");
    periodic_reporter rpt;

    for(;;)
    {
        auto r = DSK_WAIT writeFileQueue.dequeue();

        if(has_err(r))
        {
            if(get_err(r) == errc::end_reached)
            {
                break;
            }

            DSK_THROW(get_err(r));
        }

        auto cnterScope = cnter.begin_count_scope();
        size_t n = DSK_TRY file.write(get_val(r));
        cnterScope.add_and_end(B_(n));
    }

    DSK_RETURN();
}


int main(/*int argc, char* argv[]*/)
{
    stream_file srcFile, dstFile;

    if(auto r = srcFile.open("E:\\latest-all.json.gz", file_base::read_only);
       has_err(r))
    {
        stdout_report("Failed to open srcFile: ", get_err_msg(r));
        return -1;
    }

    if(auto r = dstFile.open("Z:\\latest-all.json.zstd", file_base::create | file_base::exclusive | file_base::write_only);
       has_err(r))
    {
        stdout_report("Failed to open srcFile: ", get_err_msg(r));
        return -1;
    }

    DSK_DEFAULT_IO_SCHEDULER.start();

    auto r = sync_wait(until_first_failed
    (
        run_on(DSK_DEFAULT_IO_SCHEDULER, read_file(mut_move(srcFile))),
        run_on(DSK_DEFAULT_IO_SCHEDULER, decompress()),
        run_on(DSK_DEFAULT_IO_SCHEDULER, compress()),
        run_on(DSK_DEFAULT_IO_SCHEDULER, write_file(mut_move(dstFile)))
    ));

    if(! has_err(r))
    {
        stdout_report("\n\nProcessing failed: ",
                      visit_var(get_val(r), [](auto& r){ return get_err_msg(r); }));
        return -1;
    }

    return 0;
}
