// Halley codegen version 123
#pragma once

#include <halley.hpp>


class TerminateScriptsWithTagSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 1 };
	static const constexpr char* messageName{ "TerminateScriptsWithTag" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::AllClients };
	using ReturnType = void;

	Halley::EntityId scriptableId{};
	Halley::String tag{};

	TerminateScriptsWithTagSystemMessage() {
	}

	TerminateScriptsWithTagSystemMessage(Halley::EntityId scriptableId, Halley::String tag)
		: scriptableId(std::move(scriptableId))
		, tag(std::move(tag))
	{
	}

	size_t getSize() const override final {
		return sizeof(TerminateScriptsWithTagSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << scriptableId;
		s << tag;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> scriptableId;
		s >> tag;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(scriptableId)>::deserialize(scriptableId, Halley::EntityId{}, context, node, "", "scriptableId", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tag)>::deserialize(tag, Halley::String{}, context, node, "", "tag", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
