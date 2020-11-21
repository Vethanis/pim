#include "rendering/framegraph/framegraph.hpp"
#include "rendering/vulkan/vkr_buffer.h"

namespace vkr
{
    BufferTable::~BufferTable()
    {
        for (i32 i : m_ids)
        {
            Buffer& buf = m_buffers[i];
            for (i32 j = 0; j < kFramesInFlight; ++j)
            {
                vkrBuffer_Del(&buf.buffers[j]);
            }
        }
    }

    void BufferTable::Update()
    {
        u32 const *const pim_noalias frames = m_frames.begin();
        Buffer *const pim_noalias buffers = m_buffers.begin();
        Hash64 *const pim_noalias names = m_names.begin();
        BufferInfo *const pim_noalias infos = m_infos.begin();
        const u32 curFrame = time_framecount();

        for (i32 i : m_ids)
        {
            u32 duration = (curFrame - frames[i]);
            if ((duration > kFramesInFlight))
            {
                m_ids.FreeAt(i);
                Buffer& buf = buffers[i];
                for (i32 j = 0; j < kFramesInFlight; ++j)
                {
                    vkrBuffer_Del(&buf.buffers[j]);
                }
                names[i] = {};
                infos[i] = {};
            }
        }
    }

    GenId BufferTable::Alloc(const BufferInfo& info, const char* name)
    {

    }

    bool BufferTable::Free(GenId id)
    {

    }

    // ------------------------------------------------------------------------

    ImageTable::~ImageTable()
    {

    }

    void ImageTable::Update()
    {

    }

    GenId ImageTable::Alloc(const ImageInfo& info, const char* name)
    {

    }

    bool ImageTable::Free(GenId id)
    {

    }
};
