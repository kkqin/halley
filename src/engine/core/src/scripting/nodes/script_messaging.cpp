#include "script_messaging.h"

#include "halley/support/logger.h"
#include "halley/scripting/script_message.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSendMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::ScriptMessageType", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 8>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };

	const auto msgType = ScriptMessageType(node.getSettings()["message"]);

	return gsl::span<const IScriptNodeType::PinType>(data).subspan(0, 4 + msgType.nParams);
}

std::pair<String, Vector<ColourOverride>> ScriptSendMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto msgType = ScriptMessageType(node.getSettings()["message"]);

	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(msgType.message, settingColour);
	str.append(" (");

	for (int i = 0; i < msgType.nParams; ++i) {
		if (i != 0) {
			str.append(", ");
		}
		str.append(getConnectedNodeName(world, node, graph, 4 + i), parameterColour);
	}

	str.append(") to script ");
	str.append(msgType.script, settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);

	const auto delayStr = getConnectedNodeName(world, node, graph, 3);
	if (delayStr != "<empty>") {
		str.append(" after ");
		str.append(delayStr + " s", settingColour);
	}

	return str.moveResults();
}

String ScriptSendMessage::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 3) {
		return "Delay time";
	} else if (elementIdx >= 4) {
		return "Parameter #" + toString(elementIdx - 3);
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

IScriptNodeType::Result ScriptSendMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	ScriptMessage msg;
	msg.delay = readDataPin(environment, node, 3).asFloat(0.0f);
	ScriptMessageType& msgType = msg.type;
	msgType = ScriptMessageType(node.getSettings()["message"]);

	if (msgType.nParams > 0) {
		msg.params = ConfigNode::SequenceType();
		for (int i = 0; i < msgType.nParams; ++i) {
			msg.params.asSequence().push_back(readDataPin(environment, node, 4 + i));
		}
	}

	const auto entityId = readEntityId(environment, node, 2);
	environment.sendScriptMessage(entityId, std::move(msg));
	
	return Result(ScriptNodeExecutionState::Done);
}

ConfigNode ScriptReceiveMessageData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["curArgs"] = curArgs;
	result["hasMessageActive"] = hasMessageActive;
	return result;
}


Vector<IScriptNodeType::SettingType> ScriptReceiveMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::String", Vector<String>{""} },
		SettingType{ "nParams", "Halley::Range<int, 0, 4>", Vector<String>{"0"} },
		SettingType{ "allowSpawning", "bool", Vector<String>{"false"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptReceiveMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	const int nParams = node.getSettings()["nParams"].asInt(0);

	return gsl::span<const PinType>(data).subspan(0, 1 + nParams);
}

std::pair<String, Vector<ColourOverride>> ScriptReceiveMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("On receive message ");
	str.append(node.getSettings()["message"].asString(""), settingColour);
	return str.moveResults();
}

String ScriptReceiveMessage::getPinDescription(const ScriptGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx >= 1) {
		return "Parameter #" + toString(static_cast<int>(elementIdx));
	} else {
		return ScriptNodeTypeBase<ScriptReceiveMessageData>::getPinDescription(node, element, elementIdx);
	}
}

String ScriptReceiveMessage::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx >= 1) {
		return "msg.param" + toString(static_cast<int>(elementIdx));
	} else {
		return ScriptNodeTypeBase<ScriptReceiveMessageData>::getShortDescription(world, node, graph, elementIdx);
	}
}

String ScriptReceiveMessage::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["message"].asString("");
}

bool ScriptReceiveMessage::hasDestructor(const ScriptGraphNode& node) const
{
	return true;
}

bool ScriptReceiveMessage::showDestructor() const
{
	return false;
}

std::optional<Vector2f> ScriptReceiveMessage::getNodeSize(const BaseGraphNode& node, float curZoom) const
{
	return Vector2f(120, 60);
}

void ScriptReceiveMessage::doInitData(ScriptReceiveMessageData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() != ConfigNodeType::Undefined) {
		data.curArgs = nodeData["curArgs"];
		data.hasMessageActive = nodeData["hasMessageActive"].asBool(false);
	}
}

IScriptNodeType::Result ScriptReceiveMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const
{
	return Result(ScriptNodeExecutionState::Done);
}

ConfigNode ScriptReceiveMessage::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptReceiveMessageData& data) const
{
	const size_t argN = pinN - 1;
	if (!data.hasMessageActive || data.curArgs.getType() == ConfigNodeType::Undefined || argN >= data.curArgs.asSequence().size()) {
		return ConfigNode();
	}
	return ConfigNode(data.curArgs.asSequence()[argN]);
}

void ScriptReceiveMessage::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const
{
	data.hasMessageActive = false;
	data.curArgs = ConfigNode();
}

bool ScriptReceiveMessage::canReceiveMessage(const ScriptGraphNode& node, const String& messageId, bool requiresSpawningScript) const
{
	if (messageId != node.getSettings()["message"].asString("")) {
		return false;
	}

	if (requiresSpawningScript && !node.getSettings()["allowSpawning"].asBool(false)) {
		return false;
	}

	return true;
}

bool ScriptReceiveMessage::tryReceiveMessage(const ScriptGraphNode& node, ScriptReceiveMessageData& data, ScriptMessage& msg) const
{
	Expects(msg.type.message == node.getSettings()["message"].asString(""));

	if (data.hasMessageActive) {
		return false;
	}

	data.hasMessageActive = true;
	data.curArgs = std::move(msg.params);
	return true;
}

std::pair<String, int> ScriptReceiveMessage::getMessageIdAndParams(const ScriptGraphNode& node) const
{
	return { node.getSettings()["message"].asString(""), node.getSettings()["nParams"].asInt(0) };
}



Vector<IScriptNodeType::SettingType> ScriptSendSystemMessage::getSettingTypes() const
{
	return {
		SettingType{ "system", "Halley::System", Vector<String>{""} },
		SettingType{ "message", "Halley::SystemMessageType", Vector<String>{""} },
	};
}

namespace {
	constexpr static size_t maxMsgParams = 5;
}

gsl::span<const IScriptNodeType::PinType> ScriptSendSystemMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	const auto msgType = ScriptSystemMessageType(node.getSettings()["message"]);

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2 + maxMsgParams>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return gsl::span<const PinType>(data).subspan(0, 2 + std::min(msgType.members.size(), maxMsgParams));
}

std::pair<String, Vector<ColourOverride>> ScriptSendSystemMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto msgType = ScriptSystemMessageType(node.getSettings()["message"]);

	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(msgType.message, settingColour);
	str.append(" (");

	size_t i = 0;
	for (const auto& m: msgType.members) {
		if (i != 0) {
			str.append(", ");
		}
		str.append(m + " = ");
		str.append(getConnectedNodeName(world, node, graph, 2 + i), parameterColour);
		++i;

		if (i == maxMsgParams) {
			break;
		}
	}

	str.append(") to system ");
	str.append(node.getSettings()["system"].asString(""), settingColour);
	return str.moveResults();
}

String ScriptSendSystemMessage::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx >= 2) {
		const auto msgType = ScriptSystemMessageType(node.getSettings()["message"]);
		return msgType.members.at(elementIdx - 2);
	}

	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

IScriptNodeType::Result ScriptSendSystemMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto msgType = ScriptSystemMessageType(node.getSettings()["message"]);
	const auto targetSystem = node.getSettings()["system"].asString("");

	auto args = ConfigNode::MapType();
	size_t i = 0;
	for (const auto& m: msgType.members) {
		args[m] = readDataPin(environment, node, 2 + i);
		++i;

		if (i == maxMsgParams) {
			break;
		}
	}
	environment.sendSystemMessage(ScriptEnvironment::SystemMessageData{ targetSystem, msgType.message, std::move(args) });

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptSendEntityMessage::getSettingTypes() const
{
	return {
		SettingType{ "message", "Halley::EntityMessageType", Vector<String>{""} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSendEntityMessage::getPinConfiguration(const ScriptGraphNode& node) const
{
	const auto msgType = ScriptEntityMessageType(node.getSettings()["message"]);

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3 + maxMsgParams>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return gsl::span<const PinType>(data).subspan(0, 3 + std::min(msgType.members.size(), maxMsgParams));
}

std::pair<String, Vector<ColourOverride>> ScriptSendEntityMessage::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto msgType = ScriptEntityMessageType(node.getSettings()["message"]);
	
	auto str = ColourStringBuilder(true);
	str.append("Send message ");
	str.append(msgType.message, settingColour);
	str.append(" (");

	size_t i = 0;
	for (const auto& m: msgType.members) {
		if (i != 0) {
			str.append(", ");
		}
		str.append(m + " = ");
		str.append(getConnectedNodeName(world, node, graph, 3 + i), parameterColour);
		++i;

		if (i == maxMsgParams) {
			break;
		}
	}

	str.append(") to entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSendEntityMessage::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto target = readEntityId(environment, node, 2);
	const auto msgType = ScriptEntityMessageType(node.getSettings()["message"]);
	
	auto args = ConfigNode::MapType();
	size_t i = 0;
	for (const auto& m: msgType.members) {
		args[m] = readDataPin(environment, node, 3 + i);
		++i;
		
		if (i == maxMsgParams) {
			break;
		}
	}
	environment.sendEntityMessage(ScriptEnvironment::EntityMessageData{ target, msgType.message, std::move(args) });

	return Result(ScriptNodeExecutionState::Done);
}
