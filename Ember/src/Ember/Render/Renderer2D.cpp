#include "ebpch.h"
#include "Renderer2D.h"

#include "Ember/Core/Core.h"
#include "Ember/Math/Math.h"

#include "VertexArray.h"
#include "Buffer.h"
#include "Shader.h"
#include "RenderAction.h"

namespace Ember {

	struct QuadVertex
	{
		Vector3f Position;
		Vector4f Color;
		// TODO: Texture coords
	};

	struct RendererData {
		static const unsigned int MaxQuads = 1024;
		static const unsigned int MaxVertices = MaxQuads * 4;
		static const unsigned int MaxIndices = MaxQuads * 6;

		// Quads
		SharedPtr<VertexArray> QuadVertexArray;
		SharedPtr<VertexBuffer<float>> QuadVertexBuffer;
		SharedPtr<IndexBuffer> QuadIndexBuffer;
		SharedPtr<Shader> QuadShader;

		QuadVertex* QuadBufferStart;
		QuadVertex* QuadBufferCurrent;

		Vector3f QuadVertexPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f},
		};

		unsigned int QuadIndicesInBatch;

		Matrix4f ViewProjectionMatrix = Matrix4f(1.0f);

		unsigned int DrawCallsPerScene = 0;
	};

	static ScopedPtr<RendererData> s_RendererData;


	void Renderer2D::Init()
	{
		s_RendererData = ScopedPtr<RendererData>::Create();

		s_RendererData->QuadBufferStart = new QuadVertex[s_RendererData->MaxVertices];

		s_RendererData->QuadVertexArray = VertexArray::Create();
		s_RendererData->QuadVertexBuffer = VertexBuffer<float>::Create((unsigned int)(s_RendererData->MaxVertices * sizeof(QuadVertex)));
		s_RendererData->QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "v_Position" },
			{ ShaderDataType::Float4, "v_Color" }
			});

		// Generic index buffer population
		auto quadIndexBufferData = new unsigned int[s_RendererData->MaxIndices];
		unsigned int vertexOffset = 0;
		for (unsigned int i = 0; i < s_RendererData->MaxIndices; i += 6)
		{
			quadIndexBufferData[i + 0] = vertexOffset + 0;
			quadIndexBufferData[i + 1] = vertexOffset + 1;
			quadIndexBufferData[i + 2] = vertexOffset + 2;

			quadIndexBufferData[i + 3] = vertexOffset + 2;
			quadIndexBufferData[i + 4] = vertexOffset + 3;
			quadIndexBufferData[i + 5] = vertexOffset + 0;

			vertexOffset += 4;
		}

		s_RendererData->QuadIndexBuffer = IndexBuffer::Create({ quadIndexBufferData, s_RendererData->MaxIndices });
		s_RendererData->QuadVertexArray->SetBuffer(s_RendererData->QuadVertexBuffer, s_RendererData->QuadIndexBuffer);
		delete[] quadIndexBufferData;

		s_RendererData->QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");

	}

	void Renderer2D::Shutdown()
	{
		delete[] s_RendererData->QuadBufferStart;
	}

	void Renderer2D::StartBatch()
	{
		s_RendererData->QuadIndicesInBatch = 0;
		s_RendererData->QuadBufferCurrent = s_RendererData->QuadBufferStart;
	}

	void Renderer2D::FlushBatch()
	{
		s_RendererData->DrawCallsPerScene++;

		if (s_RendererData->QuadIndicesInBatch)
		{
			// TODO: Update VertexBuffer to not be templated and to simplify all of this
			auto quadCount = s_RendererData->QuadBufferCurrent - s_RendererData->QuadBufferStart;
			auto floatPtr = reinterpret_cast<const float*>(s_RendererData->QuadBufferStart);
			auto floatCount = quadCount * sizeof(QuadVertex) / sizeof(float);
			s_RendererData->QuadVertexBuffer->SetData({ floatPtr, floatCount });

			s_RendererData->QuadShader->Bind();
			RenderAction::DrawIndexed(s_RendererData->QuadVertexArray, s_RendererData->QuadIndicesInBatch);
		}

	}

	void Renderer2D::NextBatch()
	{
		FlushBatch();
		StartBatch();
	}

	void Renderer2D::BeginFrame(OrthographicCamera& camera)
	{
		s_RendererData->DrawCallsPerScene = 0;

		s_RendererData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
		s_RendererData->QuadShader->Bind();
		s_RendererData->QuadShader->SetMatrix4("u_ViewProjection", s_RendererData->ViewProjectionMatrix);

		StartBatch();
	}

	void Renderer2D::EndFrame()
	{
		FlushBatch();
		EB_CORE_TRACE("Draw calls per scene: {}", s_RendererData->DrawCallsPerScene);
	}

	void Renderer2D::DrawQuad(const Vector2f& position, const Vector2f& size, const Vector4f& color)
	{
		Matrix4f transform = Math::Translate(Vector3f(position.x, position.y, 0.0f))
			* Math::Scale(Vector3f(size.x, size.y, 1.0f));
		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const Matrix4f& transform, const Vector4f& color)
	{
		if (s_RendererData->QuadIndicesInBatch >= s_RendererData->MaxIndices)
		{
			EB_CORE_TRACE("Indicie overload, calling next batch!");
			NextBatch();
		}

		for (unsigned int i = 0; i < 4; i++)
		{
			s_RendererData->QuadBufferCurrent->Position = transform * s_RendererData->QuadVertexPositions[i];
			s_RendererData->QuadBufferCurrent->Color = color;
			s_RendererData->QuadBufferCurrent++;
		}

		s_RendererData->QuadIndicesInBatch += 6;
	}

}