#pragma once

#include <dsk/util/atomic.hpp>
#include <dsk/util/vector.hpp>
#include <dsk/util/stringify.hpp>
#include <grpcpp/create_channel.h>


namespace dsk{


struct grpc_channel_pool_cfg
{
    std::string target;
    uint32_t channels;
    uint32_t streamsPerChannel = 100;
    grpc::ChannelArguments args{};
    std::shared_ptr<grpc::ChannelCredentials> creds = grpc::InsecureChannelCredentials();

    bool valid() const noexcept
    {
        return target.size() && channels && streamsPerChannel && creds;
    }
};


template<class Alloc = DSK_DEFAULT_ALLOCATOR<void>>
class grpc_channel_pool_t
{
public:
    using allocator_type = Alloc;

private:
    grpc_channel_pool_cfg                         _cfg;
    vector<std::shared_ptr<grpc::Channel>, Alloc> _chs;
    atomic<uint32_t>                              _next{0};

public:
    explicit grpc_channel_pool_t(grpc_channel_pool_cfg const& cfg)
        : _cfg(cfg)
    {
        DSK_ASSERT(_cfg.valid());

        auto const key = cat_as_str<std::string>("ch_", _cfg.target);

        _chs.reserve(_cfg.channels);

        for(uint32_t i = 0; i < _cfg.channels; ++i)
        {
            grpc::ChannelArguments args(_cfg.args);

            args.SetInt(key, i); // prevent grpc from sharing connection between channels with same args.

            _chs.emplace_back(grpc::CreateCustomChannel(_cfg.target, _cfg.creds, args));
        }
    }

    auto& cfg() const noexcept { return _cfg; }

    uint32_t size() const noexcept { return _chs.size(); }

    auto& get() noexcept
    {
        uint32_t const i = _next.fetch_add(1, memory_order_relaxed) / _cfg.streamsPerChannel % _chs.size();
        return _chs[i];
    }

    auto& get(uint32_t i) noexcept
    {
        DSK_ASSERT(i < _chs.size());
        return _chs[i];
    }
};


using grpc_channel_pool = grpc_channel_pool_t<>;


} // namespace dsk
