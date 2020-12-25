#ifndef CALLER_DELIMITERBASEDFRAMEDECODER_HPP
#define CALLER_DELIMITERBASEDFRAMEDECODER_HPP

#include <caller/call/pipelinereadstage.hpp>
#include <caller/common/ringbuffer.hpp>

CALLER_BEGIN

template<typename DelimiterType, typename LengthType>
class CALLER_DLL_EXPORT DelimiterBasedFrameDecoder : public PipelineReadStage
{
public:
    DelimiterBasedFrameDecoder(size_t minLength,
                                 size_t lengthOffset,
                                 DelimiterType leftDelimiter,
                                 DelimiterType rightDelimiter,
                                 size_t bufferSize)
        : _M_MinLength(minLength) , _M_OffsetLength(lengthOffset),
          _M_LeftDelimiter(leftDelimiter), _M_RightDelimiter(rightDelimiter),
          _M_RingBuffer(bufferSize) {}

    virtual ~DelimiterBasedFrameDecoder() override{}
public:
    virtual void handleRead(PipelineContextPtr context, ByteBuffer buffer, const any &object) override {
        UNUSED(object);

        size_t totalWriteBytes = 0;

        auto   next = nextStage();
        if (next == nullptr) {
            return;
        }

        while (totalWriteBytes < buffer.length()) {
            size_t writeBytes = _M_RingBuffer.put(reinterpret_cast<RingBuffer::Element*>(buffer.data() + totalWriteBytes),
                                                  buffer.length() - totalWriteBytes);
            totalWriteBytes  += writeBytes;

            while (unpack(_M_PackBuffer)) {
                any object = decode(context, _M_PackBuffer);
                if (object.has_value()) {
                    next->handleRead(context, buffer, object);
                }
            }
        }
    }
protected:
    virtual any decode(PipelineContextPtr context, ByteBuffer pack) = 0;
private:
    bool    unpack(ByteBuffer &buffer) {
        DelimiterType delimiter;
        DelimiterType rightDelimiter;
        LengthType    length;

        while (true) {
            if (_M_RingBuffer.copyTo(reinterpret_cast<RingBuffer::Element*>(&delimiter), sizeof(delimiter)) != sizeof(delimiter)) {
                return false;
            }

            delimiter = fromBigEndian(delimiter);

            if (delimiter != _M_LeftDelimiter) {
                _M_RingBuffer.move(sizeof(DelimiterType));
                continue;
            }


            // 如果大小不足最小长度，那么没有意义，不解析
            if (_M_RingBuffer.length() < _M_MinLength) {
                return false;
            }

            // 如果长度不足或者读取失败
            if (_M_RingBuffer.copyTo(reinterpret_cast<RingBuffer::Element*>(&length),
                                     sizeof(LengthType), sizeof(delimiter)) != sizeof(LengthType)) {
                 return false;
            }


            length = fromBigEndian(length);

            // 如果长度不足,那么退出，位置还在开始标识位
            if (_M_RingBuffer.length() < length) {
                return false;
            }

            // 如果获取结束标识符符失败，那么退出，复位（检查有点多余）
            if (_M_RingBuffer.copyTo(
                        reinterpret_cast<RingBuffer::Element*>(&rightDelimiter),
                        sizeof(DelimiterType),
                        length - sizeof(DelimiterType)) != sizeof(DelimiterType)
            ) {
                return false;
            }

            // 检查结束符是否符合，不符合的话，那么复位移动到下一个位置
            rightDelimiter = fromBigEndian(rightDelimiter);
            if (rightDelimiter != _M_RightDelimiter) {
                _M_RingBuffer.moveToNext();
                continue;
            }

            buffer.resize(length);
            _M_RingBuffer.take(reinterpret_cast<RingBuffer::Element*>(buffer.data()), buffer.length());
            return true;
        }
    }
private:
    size_t          _M_MinLength;
    size_t          _M_OffsetLength;
    DelimiterType   _M_LeftDelimiter;
    DelimiterType   _M_RightDelimiter;
    ByteBuffer      _M_PackBuffer;
    RingBuffer      _M_RingBuffer;
};

CALLER_END

#endif // CALLER_DELIMITERBASEDFRAMEDECODER_HPP