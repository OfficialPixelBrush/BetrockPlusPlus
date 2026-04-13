/* PIXNBT
 * Written by Pixel Brush (2025/2026)
 * Single-file version made in 2026
 *
 * LICENSE
 * Please just credit me, will you?
 *
 * FEATURES
 * Makes use of a few C++ conveniences to make it feel
 * right at home with other C++ code, though this could
 * definitely be cleaner.
 *
 * LIMITATIONS
 * Due to me not really wanting to write a custom
 * memory allocator or deflate library
 * this project makes use of a few standard libraries
 * and libdeflate
 */

#pragma once
#include <libdeflate.h>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <cstdint>

// Tag Byte Representation
enum NbtTags {
    TAG_END         =  0,
    TAG_BYTE        =  1,
    TAG_SHORT       =  2,
    TAG_INT         =  3,
    TAG_LONG        =  4,
    TAG_FLOAT       =  5,
    TAG_DOUBLE      =  6,
    TAG_BYTE_ARRAY  =  7,
    TAG_STRING      =  8,
    TAG_LIST        =  9,
    TAG_COMPOUND    = 10,
    TAG_INT_ARRAY   = 11
};

enum NbtCompressionAlgorithm : uint8_t {
    NBT_GZIP,
    NBT_ZLIB,
    NBT_UNCOMPRESSED
};

// Functions for swapping endianness
static uint16_t NbtSwap16(uint16_t value);
static uint32_t NbtSwap32(uint32_t value);
static uint64_t NbtSwap64(uint64_t value);

// -- Tag base class --
class NbtTag {
    private:
        std::string name = "";
    protected:
        virtual void Print(std::ostream& os) const {
            os << "(Tag) " << name << ": RAW";
        }
    public:
        virtual ~NbtTag() = default;  
        virtual uint8_t GetTagId() = 0;
        virtual void Write(std::ostringstream& stream, bool primary = true) = 0;
        virtual void Read(std::istringstream& stream) = 0;

        friend std::ostream& operator<<(std::ostream& os, const NbtTag& tag) {
            tag.Print(os);
            return os;
        }
        NbtTag(std::string pName = "") { this->name = pName; }

        std::string GetName() const { return name; }
        NbtTag* SetName(std::string pName) {
            this->name = pName;
            return this;
        }

        void WriteHeader(std::ostringstream& stream) {
            stream << GetTagId();
            uint16_t nameSize = NbtSwap16(static_cast<uint16_t>(name.size())); 
            stream.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
            stream << name;
        };
};

inline std::shared_ptr<NbtTag> NewNbtTag(uint8_t type, std::string name = "");
inline std::string GetTagName(int8_t type);

// Nbt Read, Write and Compression functions
inline static void NbtWrite(
    std::ofstream& stream,
    std::shared_ptr<NbtTag> tag,
    NbtCompressionAlgorithm algorithm = NBT_GZIP
);
inline static std::shared_ptr<NbtTag> NbtRead(
    std::istream& stream,
    NbtCompressionAlgorithm algorithm = NBT_GZIP,
    size_t multiplier = 10,
    size_t maxSize = 0
);

static std::vector<uint8_t> NbtCompressData(
    const std::vector<uint8_t>& inputData,
    NbtCompressionAlgorithm algorithm = NBT_GZIP,
    int32_t level = 6
);

// Implementation
#if defined(NBT_IMPLEMENTATION)
static uint16_t NbtSwap16(uint16_t value) {
    return  (value >> 8) | 
            (value << 8);
}

static uint32_t NbtSwap32(uint32_t value) {
    return  ((value >> 24) & 0x000000FF) | 
            ((value >> 8)  & 0x0000FF00) | 
            ((value << 8)  & 0x00FF0000) | 
            ((value << 24) & 0xFF000000);
}

static uint64_t NbtSwap64(uint64_t value) {
    return  ((value >> 56) & 0x00000000000000FF) |
            ((value >> 40) & 0x000000000000FF00) |
            ((value >> 24) & 0x0000000000FF0000) |
            ((value >>  8) & 0x00000000FF000000) |
            ((value <<  8) & 0x000000FF00000000) |
            ((value << 24) & 0x0000FF0000000000) |
            ((value << 40) & 0x00FF000000000000) |
            ((value << 56) & 0xFF00000000000000);
}

// --- Compound Tags ---
class CompoundNbtTag : public NbtTag {
    private:
        std::vector<std::shared_ptr<NbtTag>> tags;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Compound) " << GetName() << ": " << tags.size() << "\n";
            for (const auto& t : tags) {
                os << "\t" << *t << "\n";
            }
        }
    public:
        CompoundNbtTag(std::string pName) : NbtTag(pName){};

        // TODO: Check if a tag of the same name already exists?
        void Put(const std::string& pName, std::shared_ptr<NbtTag> tag) {
            tag->SetName(pName);
            tags.push_back(tag);
        }

        void Put(std::shared_ptr<NbtTag> tag) {
            tags.push_back(tag);
        }

        std::vector<std::shared_ptr<NbtTag>> GetTags() {
            return tags;
        }

        size_t GetNumberOfTags() {
            return tags.size();
        }
        
        std::shared_ptr<NbtTag> Get(const std::string& pName) {
            auto it = std::find_if(tags.begin(), tags.end(),
                [&](const std::shared_ptr<NbtTag>& tag) { return tag->GetName() == pName; });

            if (it == tags.end()) return nullptr;
            return *it; // Return the shared_ptr<NbtTag>
        }

        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_COMPOUND);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            for (const auto& t : tags) {
                t->Write(stream);
            }
            stream << static_cast<uint8_t>(TAG_END);
        }
        void Read(std::istringstream& stream) override {
            int32_t nTags = 0;
            while(true) {
                uint8_t type;
                stream.get(reinterpret_cast<char&>(type));
                if (type == 0) {
                    break;
                }
                uint16_t nameSize;
                stream.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
                nameSize = NbtSwap16(nameSize);
                std::string tagName(nameSize, '\0');
                stream.read(&tagName[0], nameSize);

                auto newTag = NewNbtTag(type, tagName);
                if (!newTag) {
                    break;
                }
                newTag->Read(stream);
                Put(newTag);
                nTags++;
            }
        }
};

// List Tag
class ListNbtTag : public NbtTag {
    private:
        std::vector<std::shared_ptr<NbtTag>> tags;
        uint8_t tagType = TAG_END;
    protected:
        void Print(std::ostream& os) const override {
            os << "(List) " << GetName() << ": " << tags.size() << "\n";
            for (const auto& t : tags) {
                os << "\t" << *t << "\n";
            }
        }
    public:
        ListNbtTag(std::string pName) : NbtTag(pName){};

        void Put(std::shared_ptr<NbtTag> tag) {
            // This is the first tag we put in
            if (tags.size() == 0) {
                tagType = tag->GetTagId();
            }
            if (tag->GetTagId() == tagType) {
                tags.push_back(tag);
            } else {
                std::cerr << "Non-matching Tag in list!" << "\n";
            }
        }

        std::vector<std::shared_ptr<NbtTag>> GetTags() {
            return tags;
        }

        size_t GetNumberOfTags() {
            return tags.size();
        }

        std::shared_ptr<NbtTag> Get(size_t index) {   
            return tags[index];
        }
        
        uint8_t GetTagId() override {
            return  static_cast<uint8_t>(TAG_LIST);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            // If no tags get added, defaults to value of TAG_END
            stream << static_cast<uint8_t>(tagType);
            uint32_t writtenSize = NbtSwap32(uint32_t(tags.size()));
            stream.write(reinterpret_cast<const char*>(&writtenSize), sizeof(writtenSize));
            for (size_t i = 0; i < tags.size(); ++i) {
                // TODO: Can sometimes use-after-free??
                if (tags[i]) {
                    tags[i]->Write(stream,false);
                }
            }
        }
        void Read(std::istringstream& stream) override {
            uint8_t type;
            stream.get(reinterpret_cast<char&>(type));

            uint32_t readSize;
            stream.read(reinterpret_cast<char*>(&readSize), sizeof(readSize));  // Read raw bytes for integer
            readSize = NbtSwap32(readSize);

            for (uint32_t i = 0; i < readSize; i++) {
                auto newTag = NewNbtTag(type);
                newTag->Read(stream);
                Put(newTag);
            }
        }
};

// String Tag
class StringNbtTag : public NbtTag {
    private:
        std::string data = "";
    protected:
        void Print(std::ostream& os) const override {
            os << "(String) " << GetName() << ": " << data << " (" << data.size() << ")";
        }
    public:
        StringNbtTag(std::string pName, std::string pData = "") : NbtTag(pName){ this->data = pData; }

        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_STRING);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint16_t writtenData = NbtSwap16(static_cast<uint16_t>(data.size()));
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
            stream << data;
        }
        void Read(std::istringstream& stream) override {
            uint16_t stringSize;
            if (!stream.read(reinterpret_cast<char*>(&stringSize), sizeof(stringSize))) {
                return;
            }
            stringSize = NbtSwap16(stringSize);
            std::string tempData(stringSize, '\0');
            
            if (!stream.read(tempData.data(), stringSize)) {
                std::cerr << "Failed to read from stream!" << "\n";
                return;
            }
            data = tempData;
        }
        std::string GetData() {
            return data;
        }
};

// Byte Tag
class ByteNbtTag : public NbtTag {
    private:
        int8_t data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Byte) " << GetName() << ": " << static_cast<int32_t>(data);
        }
    public:
        ByteNbtTag(std::string pName, int8_t pData = 0) : NbtTag(pName){ this->data = pData; }

        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_BYTE);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            stream.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }
        void Read(std::istringstream& stream) override {
            stream.get(reinterpret_cast<char&>(data));
        }
        int8_t GetData() {
            return data;
        }
};

// Byte Array Tag
class ByteArrayNbtTag : public NbtTag {
    private:
        std::vector<uint8_t> data;
    protected:
        void Print(std::ostream& os) const override {
            os << "(ByteArray) " << GetName() << ": " << data.size() << "\n";
            os << std::hex << "(";
            for (size_t i = 0; i < data.size(); ++i) {
                os << "0x" << static_cast<int32_t>(static_cast<uint8_t>(data[i]));
                if (i < data.size() - 1) {
                    os << ",";
                }
            }
            os << ")" << std::dec;
        }
    public:
        ByteArrayNbtTag(std::string pName) : NbtTag(pName){};
        // When std::array is passed
        template <size_t N>
            ByteArrayNbtTag(std::string pName, const std::array<uint8_t, N>& arr)
                : NbtTag(std::move(pName)), data(arr.begin(), arr.end()) {}
        // When std::vector is passed
        ByteArrayNbtTag(std::string pName, std::vector<uint8_t> pData) : NbtTag(pName){ this->data = pData; }

        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_BYTE_ARRAY);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint32_t writtenSize = NbtSwap32(static_cast<uint32_t>(data.size()));
            stream.write(reinterpret_cast<const char*>(&writtenSize), sizeof(writtenSize));
            stream.write(reinterpret_cast<const char*>(data.data()), static_cast<int32_t>(data.size()));
        }
        void Read(std::istringstream& stream) override {
            uint32_t readSize;
            stream.read(reinterpret_cast<char*>(&readSize), sizeof(readSize));  // Read raw bytes for integer
            readSize = NbtSwap32(readSize);

            // Reserve the size beforehand
            data.reserve(readSize);

            for (uint32_t i = 0; i < readSize; i++) {
                uint8_t readData;
                stream.get(reinterpret_cast<char&>(readData));
                data.push_back(readData);
            }
        }
        std::vector<uint8_t> GetData() {
            return data;
        }
};

// Short Tag
class ShortNbtTag : public NbtTag {
    private:
        int16_t data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Short) " << GetName() << ": " << static_cast<int32_t>(data);
        }
    public:
        ShortNbtTag(std::string pName, int16_t pData = 0) : NbtTag(pName){ this->data = pData; }
        
        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_SHORT);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint16_t writtenData = NbtSwap16(static_cast<uint16_t>(data));
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
        }
        void Read(std::istringstream& stream) override {
            uint16_t rawData;
            stream.read(reinterpret_cast<char*>(&rawData), sizeof(rawData));  // Read raw bytes for integer
            data = static_cast<int16_t>(NbtSwap16(rawData));
        }
        int16_t GetData() {
            return data;
        }
};

// Integer Tag
class IntNbtTag : public NbtTag {
    private:
        int32_t data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Int) " << GetName() << ": " << static_cast<int32_t>(data);
        }
    public:
        IntNbtTag(std::string pName, int32_t pData = 0) : NbtTag(pName){ this->data = pData; }
        
        uint8_t GetTagId() override {
            return  static_cast<uint8_t>(TAG_INT);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint32_t writtenData = NbtSwap32(static_cast<uint32_t>(data));
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
        }
        void Read(std::istringstream& stream) override {
            uint32_t rawData;
            stream.read(reinterpret_cast<char*>(&rawData), sizeof(rawData));  // Read raw bytes for integer
            data = static_cast<int32_t>(NbtSwap32(rawData));
        }
        int32_t GetData() {
            return data;
        }
};

// Long Tag
class LongNbtTag : public NbtTag {
    private:
        int64_t data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Long) " << GetName() << ": " << data;
        }
    public:
        LongNbtTag(std::string pName, int64_t pData = 0) : NbtTag(pName){ this->data = pData; }
        
        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_LONG);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint64_t writtenData = NbtSwap64(static_cast<uint64_t>(data));
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
        }
        void Read(std::istringstream& stream) override {
            uint64_t rawData;
            stream.read(reinterpret_cast<char*>(&rawData), sizeof(rawData));  // Read raw bytes for integer
            data = static_cast<int64_t>(NbtSwap64(rawData));
        }
        int64_t GetData() {
            return data;
        }
};

// Float Tag
class FloatNbtTag : public NbtTag {
    private:
        float data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Float) " << GetName() << ": " << std::fixed << data << std::dec;
        }
    public:
        FloatNbtTag(std::string pName, float pData = 0) : NbtTag(pName){ this->data = pData; }
        
        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_FLOAT);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint32_t writtenData = std::bit_cast<uint32_t>(data);
            writtenData = NbtSwap32(writtenData);  // Swap bytes if needed
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
        }
        void Read(std::istringstream& stream) override {
            uint32_t rawData;
            stream.read(reinterpret_cast<char*>(&rawData), sizeof(rawData));  // Read raw bytes for integer
            rawData = NbtSwap32(rawData);
            data = std::bit_cast<float>(rawData);
        }
        float GetData() {
            return data;
        }
};

// Double Tag
class DoubleNbtTag : public NbtTag {
    private:
        double data = 0;
    protected:
        void Print(std::ostream& os) const override {
            os << "(Double) " << GetName() << ": " << std::fixed << data << std::dec;
        }
    public:
        DoubleNbtTag(std::string pName, double pData = 0.0) : NbtTag(pName){ this->data = pData; }
        
        uint8_t GetTagId() override {
            return static_cast<uint8_t>(TAG_DOUBLE);
        }
        void Write(std::ostringstream& stream, bool primary = true) override {
            if (primary) {
                WriteHeader(stream);
            }
            uint64_t writtenData = std::bit_cast<uint64_t>(data);
            writtenData = NbtSwap64(writtenData);  // Swap bytes if needed
            stream.write(reinterpret_cast<const char*>(&writtenData), sizeof(writtenData));
        }
        void Read(std::istringstream& stream) override {
            uint64_t rawData;
            stream.read(reinterpret_cast<char*>(&rawData), sizeof(rawData));  // Read raw bytes for integer
            rawData = NbtSwap64(rawData);
            data = std::bit_cast<double>(rawData);
        }
        double GetData() {
            return data;
        }
};

// -- Shared Functionality for all Tags
inline std::string GetTagName(int8_t type) {
    switch(type) {
        case TAG_END:
            return "TAG_End";
        case TAG_BYTE:
            return "Tag_Byte";
        case TAG_SHORT:
            return "TAG_Short";
        case TAG_INT:
            return "TAG_Int";
        case TAG_LONG:
            return "TAG_Long";
        case TAG_FLOAT:
            return "TAG_Float";
        case TAG_BYTE_ARRAY:
            return "TAG_Byte_Array";
        case TAG_INT_ARRAY:
            return "TAG_Int_Array";
        case TAG_STRING:
            return "TAG_String";
        case TAG_LIST:
            return "TAG_List";
        case TAG_COMPOUND:
            return "TAG_Compound";
    }
    return "UNKNOWN";
}

inline std::shared_ptr<NbtTag> NewNbtTag(uint8_t type, std::string name) {
    switch(type) {
        case TAG_BYTE:
            return std::make_shared<ByteNbtTag>(name);
        case TAG_SHORT:
            return std::make_shared<ShortNbtTag>(name);
        case TAG_INT:
            return std::make_shared<IntNbtTag>(name);
        case TAG_LONG:
            return std::make_shared<LongNbtTag>(name);
        case TAG_FLOAT:
            return std::make_shared<FloatNbtTag>(name);
        case TAG_DOUBLE:
            return std::make_shared<DoubleNbtTag>(name);
        case TAG_BYTE_ARRAY:
            return std::make_shared<ByteArrayNbtTag>(name);
        case TAG_STRING:
            return std::make_shared<StringNbtTag>(name);
        case TAG_LIST:
            return std::make_shared<ListNbtTag>(name);
        case TAG_COMPOUND:
            return std::make_shared<CompoundNbtTag>(name);
        default:
            return nullptr;
    }
}

// --- Generic NBT Read/Write Stuff ---
std::vector<uint8_t> NbtCompressData(const std::vector<uint8_t>& inputData, NbtCompressionAlgorithm algorithm, int32_t level) {
    libdeflate_compressor* compressor = libdeflate_alloc_compressor(level);
    if (!compressor) {
        throw std::runtime_error("Failed to allocate libdeflate compressor");
    }

    // Allocate max possible compressed size
    size_t maxCompressedSize;
    size_t actualSize;
    std::vector<uint8_t> compressedData;
    if (algorithm == NBT_GZIP) {
        maxCompressedSize = libdeflate_gzip_compress_bound(compressor, inputData.size());
        compressedData.resize(maxCompressedSize);

        // Perform compression
        actualSize = libdeflate_gzip_compress(compressor, 
                                                    inputData.data(), inputData.size(), 
                                                    compressedData.data(), compressedData.size());
    } else if (algorithm == NBT_ZLIB) {
        maxCompressedSize = libdeflate_zlib_compress_bound(compressor, inputData.size());
        compressedData.resize(maxCompressedSize);

        // Perform compression
        actualSize = libdeflate_zlib_compress(compressor, 
                                                    inputData.data(), inputData.size(), 
                                                    compressedData.data(), compressedData.size());
    } else {
        maxCompressedSize = inputData.size();
        actualSize = inputData.size();
        compressedData = inputData;
    }

    libdeflate_free_compressor(compressor);

    if (actualSize == 0) {
        throw std::runtime_error("Compression failed");
    }

    compressedData.resize(actualSize);  // Resize buffer to actual size
    return compressedData;
}

inline void NbtWrite(std::ofstream& stream, std::shared_ptr<NbtTag> tag, NbtCompressionAlgorithm algorithm) {
    if (!stream.is_open()) {
        std::cerr << "Error: Failed to open stream or invalid stream!" << "\n";
        return;
    }
    // Creat temporary buffer
    std::ostringstream buffer(std::ios::binary);
	tag->Write(buffer,true);
    // Write to stream
    if (algorithm == NBT_UNCOMPRESSED) {
        stream.write(buffer.str().c_str(), static_cast<std::streamsize>(buffer.str().size()));
    } else {
        std::string str = buffer.str();
        std::vector<uint8_t> inputData(str.begin(), str.end());
        std::vector<uint8_t> compressedData = NbtCompressData(inputData,algorithm);

        stream.write(reinterpret_cast<const char*>(compressedData.data()), static_cast<std::streamsize>(compressedData.size()));
    }
}

inline std::shared_ptr<NbtTag> NbtRead(std::istream& stream, NbtCompressionAlgorithm algorithm, size_t multiplier, size_t maxSize) {
    // Open the file in binary mode
    if (!stream) {
        std::cerr << "Error: Failed to open stream or invalid stream!" << "\n";
        return nullptr;
    }

    // Read the entire file into memory
    stream.seekg(0, std::ios::end);
    std::streamsize ssize = stream.tellg();
    if (ssize <= 0) {
        std::cerr << "Error: Invalid or empty stream!" << "\n";
        return nullptr;
    }
    size_t size = static_cast<size_t>(ssize);
    stream.seekg(0, std::ios::beg);

    std::vector<uint8_t> compressed_data(size);
    stream.read(reinterpret_cast<char*>(compressed_data.data()), ssize);

    // If not compressed, use the data as is
    size_t estimated_size;
    if (maxSize == 0 && multiplier != 0) {
        estimated_size = size * multiplier;
    } else {
        estimated_size = maxSize;
    }
    std::vector<uint8_t> decompressed_data(estimated_size);
    libdeflate_result result = LIBDEFLATE_SUCCESS;
    if (algorithm == NBT_UNCOMPRESSED) {
        decompressed_data.assign(compressed_data.begin(), compressed_data.end());
    } else {
        libdeflate_decompressor* decompressor = libdeflate_alloc_decompressor();
        if (!decompressor) {
            std::cerr << "Failed to allocate libdeflate decompressor!" << "\n";
            return nullptr;
        }

        size_t actual_size;
        if (algorithm == NBT_GZIP) {
            result = libdeflate_gzip_decompress(
                decompressor,
                compressed_data.data(), size,           // Input data
                decompressed_data.data(), estimated_size, // Output buffer
                &actual_size                              // Actual decompressed size
            );
        } else if (algorithm == NBT_ZLIB) {
            result = libdeflate_zlib_decompress(
                decompressor,
                compressed_data.data(), size,           // Input data
                decompressed_data.data(), estimated_size, // Output buffer
                &actual_size                              // Actual decompressed size
            );
        }

        libdeflate_free_decompressor(decompressor);

        if (result != LIBDEFLATE_SUCCESS) {
            std::cerr << "Decompression failed! (" << result << ")" << "\n";
            return nullptr;
        }

        decompressed_data.resize(actual_size); // Trim to actual decompressed size
    }

    // Append a single null byte
    // End tag of above root
    decompressed_data.push_back('\0'); 

    std::string_view decompressed_str(reinterpret_cast<const char*>(decompressed_data.data()), decompressed_data.size());
    std::istringstream iss(std::string(decompressed_str), std::ios::binary);

	auto overRoot = std::make_shared<CompoundNbtTag>("");
    overRoot->Read(iss);
    auto root = *overRoot->GetTags().begin();
    if (!root) {
        std::cerr << "No root found!" << "\n";
    }
    return root;
}
#endif // NBT_IMPLEMENTATION