#include "RenderResourceFactory.h"
#include "Material.h"
#include "Mesh.h"

#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <DiligentCore/Common/interface/StringTools.hpp>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#endif

using namespace Diligent;

namespace NexusEngine
{
#ifndef NEXUS_ENGINE_SHADER_SOURCE_DIR
#define NEXUS_ENGINE_SHADER_SOURCE_DIR "NexusEngine/shaders"
#endif

#ifndef NEXUS_ENGINE_SHADER_CACHE_DIR
#define NEXUS_ENGINE_SHADER_CACHE_DIR "NexusEngine/shadercache"
#endif

    namespace
    {
        namespace fs = std::filesystem;

        fs::path GetShaderSourceRoot()
        {
            return fs::path{NEXUS_ENGINE_SHADER_SOURCE_DIR};
        }

        fs::path GetShaderCacheRoot()
        {
            return fs::path{NEXUS_ENGINE_SHADER_CACHE_DIR};
        }

        fs::path ResolveShaderPath(const char* filePath)
        {
            fs::path path{filePath};
            if (path.is_absolute())
                return path;
            return GetShaderSourceRoot() / path;
        }

        std::string ReadTextFile(const fs::path& path)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
                return {};

            std::ostringstream buffer;
            buffer << stream.rdbuf();
            return buffer.str();
        }

        std::vector<Uint8> ReadBinaryFile(const fs::path& path)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
                return {};

            stream.seekg(0, std::ios::end);
            const auto size = static_cast<size_t>(stream.tellg());
            stream.seekg(0, std::ios::beg);

            std::vector<Uint8> bytes(size);
            if (size > 0)
            {
                stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
            }
            return bytes;
        }

        bool WriteBinaryFile(const fs::path& path, const std::vector<Uint8>& bytes)
        {
            std::error_code ec;
            fs::create_directories(path.parent_path(), ec);

            std::ofstream stream{path, std::ios::binary | std::ios::trunc};
            if (!stream)
                return false;

            if (!bytes.empty())
            {
                stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            }
            return static_cast<bool>(stream);
        }

        uint64_t HashBytes(std::string_view data)
        {
            uint64_t hash = 1469598103934665603ull;
            for (unsigned char ch : data)
            {
                hash ^= static_cast<uint64_t>(ch);
                hash *= 1099511628211ull;
            }
            return hash;
        }

        const char* GetShaderProfile(SHADER_TYPE type)
        {
            switch (type)
            {
                case SHADER_TYPE_VERTEX: return "vs_5_0";
                case SHADER_TYPE_PIXEL:  return "ps_5_0";
                default:                 return nullptr;
            }
        }

        fs::path GetCachePath(const fs::path& sourcePath, const char* entryPoint, SHADER_TYPE type)
        {
            const std::string source = ReadTextFile(sourcePath);
            const auto profile = std::string{GetShaderProfile(type) ? GetShaderProfile(type) : "unknown"};
            const auto fingerprint = HashBytes(source + "|" + entryPoint + "|" + profile);
            return GetShaderCacheRoot() / (sourcePath.filename().string() + "." + std::to_string(fingerprint) + ".cso");
        }

#if defined(_WIN32)
        bool CompileHlslToBytecode(const fs::path& sourcePath, const char* entryPoint, SHADER_TYPE type, std::vector<Uint8>& bytecode)
        {
            const char* profile = GetShaderProfile(type);
            if (!profile)
                return false;

            UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

            ID3DBlob* shaderBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;
            const HRESULT hr = D3DCompileFromFile(
                sourcePath.wstring().c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entryPoint,
                profile,
                compileFlags,
                0,
                &shaderBlob,
                &errorBlob);

            if (FAILED(hr))
            {
                if (errorBlob)
                {
                    errorBlob->Release();
                }
                if (shaderBlob)
                {
                    shaderBlob->Release();
                }
                return false;
            }

            bytecode.resize(shaderBlob->GetBufferSize());
            std::memcpy(bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

            shaderBlob->Release();
            if (errorBlob)
            {
                errorBlob->Release();
            }
            return true;
        }
#endif
    }

    size_t RenderResourceFactory::PipelineKeyHasher::operator()(const PipelineKey& key) const noexcept
    {
        const size_t h1 = std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(key.m_material));
        const size_t h2 = std::hash<std::uintptr_t>{}(reinterpret_cast<std::uintptr_t>(key.m_mesh));
        const size_t h3 = std::hash<int>{}(static_cast<int>(key.m_rtvFormat));
        const size_t h4 = std::hash<int>{}(static_cast<int>(key.m_dsvFormat));
        return (((h1 * 1315423911u) ^ h2) * 2654435761u) ^ (h3 << 1) ^ (h4 << 7);
    }

    RenderResourceFactory::RenderResourceFactory(IRenderDevice* device, IDeviceContext* context)
        : m_device(device)
        , m_context(context)
    {
    }

    RefCntAutoPtr<IShader> RenderResourceFactory::CreateShaderFromSource(
        const char* source,
        const char* entryPoint,
        SHADER_TYPE type,
        const char* name)
    {
        ShaderCreateInfo shaderCI;
        shaderCI.Source = source;
        shaderCI.EntryPoint = entryPoint;
        shaderCI.Desc.ShaderType = type;
        shaderCI.Desc.Name = name ? name : "Shader";
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        shaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;

        RefCntAutoPtr<IShader> shader;
        m_device->CreateShader(shaderCI, &shader);
        return shader;
    }

    RefCntAutoPtr<IShader> RenderResourceFactory::CreateShaderFromFile(
        const char* filePath,
        const char* entryPoint,
        SHADER_TYPE type,
        const char* name)
    {
        const auto sourcePath = ResolveShaderPath(filePath);
        const auto cachePath = GetCachePath(sourcePath, entryPoint, type);

        RefCntAutoPtr<IShader> shader;

#if defined(_WIN32)
        std::vector<Uint8> bytecode;
        if (std::filesystem::exists(cachePath))
        {
            bytecode = ReadBinaryFile(cachePath);
        }

        if (bytecode.empty())
        {
            if (!CompileHlslToBytecode(sourcePath, entryPoint, type, bytecode))
                return shader;

            WriteBinaryFile(cachePath, bytecode);
        }

        ShaderCreateInfo shaderCI;
        shaderCI.ByteCode = bytecode.data();
        shaderCI.ByteCodeSize = bytecode.size();
        shaderCI.Desc.ShaderType = type;
        shaderCI.Desc.Name = name ? name : filePath;
        m_device->CreateShader(shaderCI, &shader);
        return shader;
#else
        const std::string source = ReadTextFile(sourcePath);
        if (source.empty())
            return shader;

        ShaderCreateInfo shaderCI;
        shaderCI.Source = source.c_str();
        shaderCI.SourceLength = source.length();
        shaderCI.EntryPoint = entryPoint;
        shaderCI.Desc.ShaderType = type;
        shaderCI.Desc.Name = name ? name : filePath;
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        shaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
        m_device->CreateShader(shaderCI, &shader);
        return shader;
#endif
    }

    RefCntAutoPtr<IPipelineState> RenderResourceFactory::CreateGraphicsPipeline(
        const char* name,
        IShader* vertexShader,
        IShader* pixelShader,
        const std::vector<LayoutElement>& inputLayout,
        PRIMITIVE_TOPOLOGY topology,
        CULL_MODE cullMode,
        bool depthTestEnabled,
        bool depthWriteEnabled,
        TEXTURE_FORMAT rtvFormat,
        TEXTURE_FORMAT dsvFormat)
    {
        GraphicsPipelineStateCreateInfo psoCI;
        psoCI.PSODesc.Name = name;
        psoCI.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        // Resource layout - allow mutable constant buffers
        psoCI.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        // Shaders
        psoCI.pVS = vertexShader;
        psoCI.pPS = pixelShader;

        // Input layout
        psoCI.GraphicsPipeline.InputLayout.LayoutElements = inputLayout.data();
        psoCI.GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(inputLayout.size());

        // Primitive topology
        psoCI.GraphicsPipeline.PrimitiveTopology = topology;

        // Rasterizer state
        psoCI.GraphicsPipeline.RasterizerDesc.CullMode = cullMode;
        psoCI.GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_SOLID;
        psoCI.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;

        // Depth-stencil state
        psoCI.GraphicsPipeline.DepthStencilDesc.DepthEnable = depthTestEnabled;
        psoCI.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = depthWriteEnabled;
        psoCI.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS;

        // Blend state
        psoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = false;
        psoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask = COLOR_MASK_ALL;

        // Render target and depth-stencil formats
        psoCI.GraphicsPipeline.NumRenderTargets = 1;
        psoCI.GraphicsPipeline.RTVFormats[0] = rtvFormat;
        psoCI.GraphicsPipeline.DSVFormat = dsvFormat;

        RefCntAutoPtr<IPipelineState> pso;
        m_device->CreateGraphicsPipelineState(psoCI, &pso);

        return pso;
    }

    Material* RenderResourceFactory::CreateMaterial(
        const char* name,
        const char* vsSource,
        const char* psSource,
        const std::vector<LayoutElement>& inputLayout,
        PRIMITIVE_TOPOLOGY topology)
    {
        (void)topology;

        Material* mat = new Material();
        mat->name = name;
        mat->m_inputLayout = inputLayout;

        // Create shaders
        mat->vertexShader = CreateShaderFromSource(
            vsSource,
            "main",
            SHADER_TYPE_VERTEX,
            (std::string(name) + "_VS").c_str());

        mat->pixelShader = CreateShaderFromSource(
            psSource,
            "main",
            SHADER_TYPE_PIXEL,
            (std::string(name) + "_PS").c_str());

        return mat;
    }

    Material* RenderResourceFactory::CreateMaterialFromFiles(
        const char* name,
        const char* vsFilePath,
        const char* psFilePath,
        const std::vector<LayoutElement>& inputLayout,
        PRIMITIVE_TOPOLOGY topology)
    {
        (void)topology;

        Material* mat = new Material();
        mat->name = name;
        mat->m_inputLayout = inputLayout;

        mat->vertexShader = CreateShaderFromFile(
            vsFilePath,
            "main",
            SHADER_TYPE_VERTEX,
            (std::string{name} + "_VS").c_str());

        mat->pixelShader = CreateShaderFromFile(
            psFilePath,
            "main",
            SHADER_TYPE_PIXEL,
            (std::string{name} + "_PS").c_str());

        return mat;
    }

    RenderResourceFactory::CachedPipeline* RenderResourceFactory::GetOrCreatePipeline(
        Material* material,
        Mesh* mesh,
        TEXTURE_FORMAT rtvFormat,
        TEXTURE_FORMAT dsvFormat)
    {
        if (!material || !mesh || !material->vertexShader || !material->pixelShader)
            return nullptr;

        const PipelineKey key{ material, mesh, rtvFormat, dsvFormat };
        if (auto it = m_pipelineCache.find(key); it != m_pipelineCache.end())
            return &it->second;

        CachedPipeline cachedPipeline;
        const std::string pipelineName = material->name + "_" + mesh->name + "_RT" +
            std::to_string(static_cast<int>(rtvFormat)) + "_DS" + std::to_string(static_cast<int>(dsvFormat));

        cachedPipeline.m_pipelineState = CreateGraphicsPipeline(
            pipelineName.c_str(),
            material->vertexShader,
            material->pixelShader,
            material->m_inputLayout,
            mesh->topology,
            material->cullMode,
            material->depthTestEnabled,
            material->depthWriteEnabled,
            rtvFormat,
            dsvFormat);

        if (!cachedPipeline.m_pipelineState)
            return nullptr;

        cachedPipeline.m_pipelineState->CreateShaderResourceBinding(&cachedPipeline.m_shaderResourceBindingTemplate, true);

        if (cachedPipeline.m_shaderResourceBindingTemplate && material->materialConstantBuffer)
        {
            if (auto* var = cachedPipeline.m_shaderResourceBindingTemplate->GetVariableByName(SHADER_TYPE_VERTEX, "VSConstants"))
            {
                var->Set(material->materialConstantBuffer);
            }
        }

        auto [it, inserted] = m_pipelineCache.emplace(key, std::move(cachedPipeline));
        (void)inserted;
        return &it->second;
    }

    Material* RenderResourceFactory::CreateDefaultMaterial()
    {
        std::vector<LayoutElement> layout = {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            LayoutElement{1, 0, 3, VT_FLOAT32, False},
            LayoutElement{2, 0, 2, VT_FLOAT32, False}
        };

        Material* mat = CreateMaterialFromFiles(
            "DefaultLitMaterial",
            "DefaultLit.vs.hlsl",
            "DefaultLit.ps.hlsl",
            layout);

        if (mat && m_context)
        {
            mat->materialConstantBuffer = CreateConstantBuffer("VSConstants", 128);

            if (mat->materialConstantBuffer)
            {
                struct VSConstants
                {
                    float4x4 WorldViewProj;
                    float4x4 World;
                };

                MapHelper<VSConstants> mappedData(m_context, mat->materialConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                mappedData->World = float4x4::Identity();
                mappedData->WorldViewProj = float4x4::Identity();
            }
        }

        return mat;
    }

    Material* RenderResourceFactory::CreateUnlitMaterial()
    {
        std::vector<LayoutElement> layout = {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},  // Position
            LayoutElement{1, 0, 3, VT_FLOAT32, False},  // Normal
            LayoutElement{2, 0, 2, VT_FLOAT32, False}   // UV
        };

        Material* mat = CreateMaterialFromFiles(
            "UnlitMaterial",
            "Unlit.vs.hlsl",
            "Unlit.ps.hlsl",
            layout);

        // Create constant buffer for VSConstants (one 4x4 matrix = 64 bytes)
        if (mat)
        {
            mat->materialConstantBuffer = CreateConstantBuffer("VSConstants", 64);

            // IMPORTANT: Map buffer once to allocate GPU memory for dynamic buffers
            if (mat->materialConstantBuffer && m_context)
            {
                MapHelper<float4x4> mappedData(m_context, mat->materialConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                *mappedData = float4x4::Identity();
            }
        }

        return mat;
    }

    bool RenderResourceFactory::CompileAllShaders()
    {
        namespace fs = std::filesystem;

        std::error_code ec;
        const fs::path root = GetShaderSourceRoot();
        if (!fs::exists(root, ec))
            return false;

        bool success = true;
        for (const auto& entry : fs::recursive_directory_iterator(root, ec))
        {
            if (ec || !entry.is_regular_file())
                continue;

            const auto path = entry.path();
            const auto filename = path.filename().string();
            SHADER_TYPE shaderType = SHADER_TYPE_UNKNOWN;

            if (filename.ends_with(".vs.hlsl"))
                shaderType = SHADER_TYPE_VERTEX;
            else if (filename.ends_with(".ps.hlsl"))
                shaderType = SHADER_TYPE_PIXEL;

            if (shaderType == SHADER_TYPE_UNKNOWN)
                continue;

            auto shader = CreateShaderFromFile(path.string().c_str(), "main", shaderType, filename.c_str());
            if (!shader)
                success = false;
        }

        return success;
    }

    Mesh* RenderResourceFactory::CreateMesh(
        const char* name,
        const void* vertexData,
        Uint32 vertexCount,
        Uint32 vertexStride,
        const Uint32* indexData,
        Uint32 indexCount,
        PRIMITIVE_TOPOLOGY topology)
    {
        Mesh* mesh = new Mesh();
        mesh->name = name;
        mesh->vertexCount = vertexCount;
        mesh->indexCount = indexCount;
        mesh->vertexStride = vertexStride;
        mesh->topology = topology;

        // Create vertex buffer
        {
            BufferDesc bufDesc;
            bufDesc.Name = (std::string(name) + "_VB").c_str();
            bufDesc.Usage = USAGE_IMMUTABLE;
            bufDesc.BindFlags = BIND_VERTEX_BUFFER;
            bufDesc.Size = vertexCount * vertexStride;

            BufferData bufData;
            bufData.pData = vertexData;
            bufData.DataSize = bufDesc.Size;

            m_device->CreateBuffer(bufDesc, &bufData, &mesh->vertexBuffer);
        }

        // Create index buffer if provided
        if (indexData && indexCount > 0)
        {
            BufferDesc bufDesc;
            bufDesc.Name = (std::string(name) + "_IB").c_str();
            bufDesc.Usage = USAGE_IMMUTABLE;
            bufDesc.BindFlags = BIND_INDEX_BUFFER;
            bufDesc.Size = indexCount * sizeof(Uint32);

            BufferData bufData;
            bufData.pData = indexData;
            bufData.DataSize = bufDesc.Size;

            m_device->CreateBuffer(bufDesc, &bufData, &mesh->indexBuffer);
        }

        return mesh;
    }

    Mesh* RenderResourceFactory::CreateCubeMesh()
    {
        struct Vertex
        {
            float pos[3];
            float normal[3];
            float uv[2];
        };

        Vertex vertices[] = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            
            // Back face
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            
            // Top face
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            
            // Bottom face
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            
            // Right face
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            
            // Left face
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
        };

        Uint32 indices[] = {
            0, 1, 2,  2, 3, 0,      // Front
            4, 5, 6,  6, 7, 4,      // Back
            8, 9, 10, 10, 11, 8,    // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };

        return CreateMesh(
            "Cube",
            vertices,
            sizeof(vertices) / sizeof(Vertex),
            sizeof(Vertex),
            indices,
            sizeof(indices) / sizeof(Uint32));
    }

    Mesh* RenderResourceFactory::CreatePlaneMesh()
    {
        struct Vertex
        {
            float pos[3];
            float normal[3];
            float uv[2];
        };

        Vertex vertices[] = {
            {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
        };

        Uint32 indices[] = {
            0, 1, 2,  2, 3, 0
        };

        return CreateMesh(
            "Plane",
            vertices,
            4,
            sizeof(Vertex),
            indices,
            6);
    }

    Mesh* RenderResourceFactory::CreateSphereMesh(int segments)
    {
        struct Vertex
        {
            float pos[3];
            float normal[3];
            float uv[2];
        };

        std::vector<Vertex> vertices;
        std::vector<Uint32> indices;

        const float PI = 3.14159265359f;
        
        // Generate vertices
        for (int lat = 0; lat <= segments; ++lat)
        {
            float theta = lat * PI / segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            for (int lon = 0; lon <= segments; ++lon)
            {
                float phi = lon * 2.0f * PI / segments;
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);

                Vertex v;
                v.pos[0] = cosPhi * sinTheta * 0.5f;
                v.pos[1] = cosTheta * 0.5f;
                v.pos[2] = sinPhi * sinTheta * 0.5f;

                v.normal[0] = cosPhi * sinTheta;
                v.normal[1] = cosTheta;
                v.normal[2] = sinPhi * sinTheta;

                v.uv[0] = static_cast<float>(lon) / segments;
                v.uv[1] = static_cast<float>(lat) / segments;

                vertices.push_back(v);
            }
        }

        // Generate indices
        for (int lat = 0; lat < segments; ++lat)
        {
            for (int lon = 0; lon < segments; ++lon)
            {
                Uint32 first = lat * (segments + 1) + lon;
                Uint32 second = first + segments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        return CreateMesh(
            "Sphere",
            vertices.data(),
            static_cast<Uint32>(vertices.size()),
            sizeof(Vertex),
            indices.data(),
            static_cast<Uint32>(indices.size()));
    }

    RefCntAutoPtr<IBuffer> RenderResourceFactory::CreateConstantBuffer(
        const char* name,
        Uint32 size)
    {
        BufferDesc bufDesc;
        bufDesc.Name = name;
        bufDesc.Usage = USAGE_DYNAMIC;
        bufDesc.BindFlags = BIND_UNIFORM_BUFFER;
        bufDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        bufDesc.Size = size;

        RefCntAutoPtr<IBuffer> buffer;
        m_device->CreateBuffer(bufDesc, nullptr, &buffer);
        return buffer;
    }
} // namespace NexusEngine
