#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/vector2.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class EntitySerializationContext;

	class WorldPosition {
	public:
		Vector2f pos;
		int subWorld;
		
		constexpr WorldPosition(Vector2f pos = {}, int subWorld = 0)
			: pos(pos)
			, subWorld(subWorld)
		{
		}

		WorldPosition(const ConfigNode& node, Vector2f defaultPos = {}, int defaultSubWorld = 0);

		ConfigNode toConfigNode() const;
		String toString() const;

		WorldPosition operator+(const Vector2f& other) const;
		WorldPosition operator-(const Vector2f& other) const;

		bool operator==(const WorldPosition& other) const;
		bool operator!=(const WorldPosition& other) const;
	};

	template<>
	class ConfigNodeSerializer<WorldPosition> {
	public:
		ConfigNode serialize(const WorldPosition& target, const EntitySerializationContext& context);
		WorldPosition deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}
