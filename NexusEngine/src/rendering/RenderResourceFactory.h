#pragma once

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Common/interface/BasicMath.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Shader.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/Buffer.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace NexusEngine
{
    struct Material;
    struct Mesh;

    // Vertex format definition
    struct VertexPosNormalUV
    {
        float position[3];
        float normal[3];
        float uv[2];
    };

    struct VertexPosColorUV
    {
        float position[3];
        float color[4];
        float uv[2];
    };

    // Helper class for creating rendering resources
    class RenderResourceFactory
    {
    public:
        struct CachedPipeline
        {
            Diligent::RefCntAutoPtr<Diligent::IPipelineState> m_pipelineState;
            Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> m_shaderResourceBindingTemplate;
        };

        RenderResourceFactory(Diligent::IRenderDevice* device, Diligent::IDeviceContext* context);

        // Shader creation
        Diligent::RefCntAutoPtr<Diligent::IShader> CreateShaderFromSource(
            const char* source,
            const char* entryPoint,
            Diligent::SHADER_TYPE type,
            const char* name = nullptr);

        Diligent::RefCntAutoPtr<Diligent::IShader> CreateShaderFromFile(
            const char* filePath,
            const char* entryPoint,
            Diligent::SHADER_TYPE type,
            const char* name = nullptr);

        // Pipeline state creation
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> CreateGraphicsPipeline(
            const char* name,
            Diligent::IShader* vertexShader,
            Diligent::IShader* pixelShader,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            Diligent::CULL_MODE cullMode = Diligent::CULL_MODE_BACK,
            bool depthTestEnabled = true,
            bool depthWriteEnabled = true,
            Diligent::TEXTURE_FORMAT rtvFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB,
            Diligent::TEXTURE_FORMAT dsvFormat = Diligent::TEX_FORMAT_D32_FLOAT);

        // Material creation helpers
        Material* CreateMaterial(
            const char* name,
            const char* vsSource,
            const char* psSource,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        Material* CreateMaterialFromFiles(
            const char* name,
            const char* vsFilePath,
            const char* psFilePath,
            const std::vector<Diligent::LayoutElement>& inputLayout,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        Material* CreateDefaultMaterial();
        Material* CreateUnlitMaterial();
        bool CompileAllShaders();

        CachedPipeline* GetOrCreatePipeline(
            Material* material,
            Mesh* mesh,
            Diligent::TEXTURE_FORMAT rtvFormat,
            Diligent::TEXTURE_FORMAT dsvFormat);

        // Mesh creation helpers
        Mesh* CreateMesh(
            const char* name,
            const void* vertexData,
            Diligent::Uint32 vertexCount,
            Diligent::Uint32 vertexStride,
            const Diligent::Uint32* indexData = nullptr,
            Diligent::Uint32 indexCount = 0,
            Diligent::PRIMITIVE_TOPOLOGY topology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // Simple shapes
        Mesh* CreateCubeMesh();
        Mesh* CreatePlaneMesh();
        Mesh* CreateSphereMesh(int segments = 32);

        // Constant buffer creation
        Diligent::RefCntAutoPtr<Diligent::IBuffer> CreateConstantBuffer(
            const char* name,
            Diligent::Uint32 size);

    private:
        struct PipelineKey
        {
            const Material* m_material = nullptr;
            const Mesh* m_mesh = nullptr;
            Diligent::TEXTURE_FORMAT m_rtvFormat = Diligent::TEX_FORMAT_UNKNOWN;
            Diligent::TEXTURE_FORMAT m_dsvFormat = Diligent::TEX_FORMAT_UNKNOWN;

            bool operator==(const PipelineKey& other) const = default;
        };

        struct PipelineKeyHasher
        {
            size_t operator()(const PipelineKey& key) const noexcept;
        };

        Diligent::IRenderDevice* m_device;
        Diligent::IDeviceContext* m_context;
        std::unordered_map<PipelineKey, CachedPipeline, PipelineKeyHasher> m_pipelineCache;
    };
} // namespace NexusEngine
