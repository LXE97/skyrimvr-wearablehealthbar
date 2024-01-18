#pragma once
#include "Windows.h"

namespace helper
{
	using namespace RE;
	RE::TESForm* LookupByName(RE::FormType a_typeEnum, const char* a_name);
	RE::FormID   GetFullFormID(uint8_t a_modindex, RE::FormID a_localID);

	void CastSpellInstant(RE::Actor* a_src, RE::Actor* a_target, RE::SpellItem* sa_pell);
	void Dispel(RE::Actor* a_src, RE::Actor* a_target, RE::SpellItem* a_spell);

	float GetAVPercent(RE::Actor* a_a, RE::ActorValue a_v);
	float GetChargePercent(RE::Actor* a_a, bool isLeft);
	float GetGameHour();  //  24hr time
	float GetAmmoPercent(RE::Actor* a_a, float a_ammoCountMult);
	float GetShoutCooldownPercent(RE::Actor* a_a, float a_MaxCDTime);

	void SetGlowMult();
	void SetGlowColor(RE::NiAVObject* a_target, RE::NiColor* a_c);
	void SetSpecularMult();
	void SetSpecularColor();
	void SetTintColor();
	void SetUVCoords(RE::NiAVObject* a_target, float a_x, float a_y);

	inline RE::BSShaderProperty* GetShaderProperty(RE::NiAVObject* a_target, const char* a_node)
	{
		if (a_target)
		{
			RE::NiAVObject* geo_node;
			if (a_node) { geo_node = a_target->GetObjectByName(a_node); }
			else { geo_node = a_target; }

			if (geo_node)
			{
				if (auto geometry = geo_node->AsGeometry())
				{
					if (auto property = geometry->properties[RE::BSGeometry::States::kEffect].get())
					{
						if (auto shader = netimmerse_cast<RE::BSShaderProperty*>(property))
						{
							return shader;
						}
					}
				}
			}
		}
		return nullptr;
	}

	inline void GetVertexData(RE::NiAVObject* a_target)
	{
		if (auto geom = a_target->AsGeometry())
		{
			std::vector<uint32_t> c;
			NiUpdateData          ctx;
			if (auto tri = static_cast<BSTriShape*>(geom))
			{
				if (auto property = geom->properties[RE::BSGeometry::States::kEffect].get())
				{
					if (auto shader = netimmerse_cast<RE::BSShaderProperty*>(property))
					{
						auto data = geom->GetGeometryRuntimeData();

						shader->lastRenderPassState = std::numeric_limits<std::int32_t>::max();
						auto material =
							static_cast<RE::BSLightingShaderMaterialHairTint*>(shader->material);
						a_target->Update(ctx);
					}
				}
			}

			/*
	BSGeometryData *geomData = geom->geometryData;
	if (!geomData) return false;

	UInt64 vertexDesc = geom->vertexDesc;


	uintptr_t verts = (uintptr_t)(geomData->vertices);
	UInt16 numVerts = geom->numVertices;
	UInt8 vertexSize = (vertexDesc & 0xF) * 4;
	UInt32 colorOffset = NiSkinPartition::GetVertexAttributeOffset(vertexDesc, VertexAttribute::VA_COLOR);

	if (verts && numVerts > 0) {
		float totalAlpha = 0.f;
		for (int i = 0; i < numVerts; i++) {
			uintptr_t vert = (verts + i * vertexSize);
			UInt32 color = *(UInt32 *)(vert + colorOffset);
			UInt8 alpha0to255 = (color >> 24) & 0xff;
			float alpha = float(alpha0to255) / 255.f;
			totalAlpha += alpha;
		}
		float avgAlpha = totalAlpha / numVerts;
		_MESSAGE("%s: %.3f avg alpha", geom->m_name, avgAlpha);
		if (avgAlpha < Config::options.geometryVertexAlphaThreshold) {
			return true;
		}
	}
/*
	NiPointer<NiProperty> geomProperty = geom->m_spEffectState;
	if (!geomProperty) return false;

	BSShaderProperty *shaderProperty = DYNAMIC_CAST(geomProperty, NiProperty, BSShaderProperty);
	if (!shaderProperty) return false;

	if (!(shaderProperty->shaderFlags1 & BSShaderProperty::ShaderFlags1::kSLSF1_Vertex_Alpha)) return false;

	BSGeometryData *geomData = geom->geometryData;
	if (!geomData) return false;

	UInt64 vertexDesc = geom->vertexDesc;
	VertexFlags vertexFlags = NiSkinPartition::GetVertexFlags(vertexDesc);
	if (!(vertexFlags & VertexFlags::VF_COLORS)) return false;

	uintptr_t verts = (uintptr_t)(geomData->vertices);
	UInt16 numVerts = geom->numVertices;
	UInt8 vertexSize = (vertexDesc & 0xF) * 4;
	UInt32 colorOffset = NiSkinPartition::GetVertexAttributeOffset(vertexDesc, VertexAttribute::VA_COLOR);
*/
		}
	}

	inline void FaceCamera(RE::NiAVObject* a_target)
	{
		if (a_target)
		{
			auto vector_to_camera = RE::PlayerCharacter::GetSingleton()
										->GetNodeByName("NPC Head [Head]")
										->world.translate -
				a_target->world.translate;

			float heading = std::atan2f(vector_to_camera.y, vector_to_camera.x);

			a_target->local.rotate.SetEulerAnglesXYZ({ 0, 0, -heading });
			a_target->local.rotate =
				a_target->parent->world.rotate.Transpose() * a_target->local.rotate;
		}
	}

	void PrintPlayerModelEffects();
	void PrintPlayerShaderEffects();
}
