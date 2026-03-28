#include "RenderResourceFactory.h"
#include "Material.h"
#include "Mesh.h"

#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/GraphicsTypes.h>
#include <DiligentCore/Graphics/GraphicsTools/interface/MapHelper.hpp>
#include <DiligentCore/Common/interface/StringTools.hpp>
#include <cmath>
#include <vector>

using namespace Diligent;

namespace NexusEngine
{
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
        ShaderCreateInfo shaderCI;
        shaderCI.FilePath = filePath;
        shaderCI.EntryPoint = entryPoint;
        shaderCI.Desc.ShaderType = type;
        shaderCI.Desc.Name = name ? name : filePath;
        shaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        shaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;

        RefCntAutoPtr<IShader> shader;
        m_device->CreateShader(shaderCI, &shader);
        return shader;
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
        Material* mat = new Material();
        mat->name = name;

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

        // Create pipeline state
        if (mat->vertexShader && mat->pixelShader)
        {
            mat->pipelineState = CreateGraphicsPipeline(
                name,
                mat->vertexShader,
                mat->pixelShader,
                inputLayout,
                topology,
                mat->cullMode,
                mat->depthTestEnabled,
                mat->depthWriteEnabled);

            // Create shader resource binding template
            if (mat->pipelineState)
            {
                mat->pipelineState->CreateShaderResourceBinding(&mat->shaderResourceBindingTemplate, true);
            }
        }

        return mat;
    }

    Material* RenderResourceFactory::CreateDefaultMaterial()
    {
        const char* vsSource = R"(
cbuffer VSConstants
{
    float4x4 g_WorldViewProj;
    float4x4 g_World;
};

struct VSInput
{
    float3 Pos    : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV     : ATTRIB2;
};

struct PSInput
{
    float4 Pos    : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV     : TEXCOORD0;
    float3 WorldPos : WORLD_POS;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_WorldViewProj);
    PSIn.Normal = mul(float4(VSIn.Normal, 0.0), g_World).xyz;
    PSIn.UV = VSIn.UV;
    PSIn.WorldPos = mul(float4(VSIn.Pos, 1.0), g_World).xyz;
}
)";

        const char* psSource = R"(
struct PSInput
{
    float4 Pos    : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV     : TEXCOORD0;
    float3 WorldPos : WORLD_POS;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    // Simple directional light
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(PSIn.Normal);
    float ndl = max(dot(normal, lightDir), 0.0);

    // Base color with simple lighting
    float3 baseColor = float3(0.7, 0.7, 0.7);
    float3 ambient = baseColor * 0.3;
    float3 diffuse = baseColor * 0.7 * ndl;

    PSOut.Color = float4(ambient + diffuse, 1.0);
}
)";

        std::vector<LayoutElement> layout = {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},
            LayoutElement{1, 0, 3, VT_FLOAT32, False},
            LayoutElement{2, 0, 2, VT_FLOAT32, False}
        };

        Material* mat = CreateMaterial("DefaultLitMaterial", vsSource, psSource, layout);

        if (mat && mat->pipelineState && m_context)
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

            if (mat->shaderResourceBindingTemplate && mat->materialConstantBuffer)
            {
                auto* var = mat->shaderResourceBindingTemplate->GetVariableByName(SHADER_TYPE_VERTEX, "VSConstants");
                if (var)
                {
                    var->Set(mat->materialConstantBuffer);
                }
            }
        }

        return mat;
    }

    Material* RenderResourceFactory::CreateUnlitMaterial()
    {
        const char* vsSource = R"(
cbuffer VSConstants
{
    float4x4 g_WorldViewProj;
};

struct VSInput
{
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV : ATTRIB2;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_WorldViewProj);
    PSIn.Color = VSIn.Normal * 0.5 + 0.5;
}
)";

        const char* psSource = R"(
struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color, 1.0);
}
)";

        std::vector<LayoutElement> layout = {
            LayoutElement{0, 0, 3, VT_FLOAT32, False},  // Position
            LayoutElement{1, 0, 3, VT_FLOAT32, False},  // Normal
            LayoutElement{2, 0, 2, VT_FLOAT32, False}   // UV
        };

        Material* mat = CreateMaterial("UnlitMaterial", vsSource, psSource, layout);

        // Create constant buffer for VSConstants (one 4x4 matrix = 64 bytes)
        if (mat && mat->pipelineState)
        {
            mat->materialConstantBuffer = CreateConstantBuffer("VSConstants", 64);

            // IMPORTANT: Map buffer once to allocate GPU memory for dynamic buffers
            if (mat->materialConstantBuffer && m_context)
            {
                MapHelper<float4x4> mappedData(m_context, mat->materialConstantBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
                *mappedData = float4x4::Identity();
            }

            // Bind constant buffer to SRB
            if (mat->shaderResourceBindingTemplate && mat->materialConstantBuffer)
            {
                auto* var = mat->shaderResourceBindingTemplate->GetVariableByName(SHADER_TYPE_VERTEX, "VSConstants");
                if (var)
                {
                    var->Set(mat->materialConstantBuffer);
                }
            }
        }

        return mat;
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
