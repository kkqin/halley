#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {

	class ScriptFlowGateData : public ScriptStateData<ScriptFlowGateData> {
	public:
		ScriptFlowGateData() = default;
		ScriptFlowGateData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<bool> flowing;
	};

	class ScriptFlowGate : public ScriptNodeTypeBase<ScriptFlowGateData> {
	public:
		String getId() const override { return "flowGate"; }
		String getName() const override { return "Flow Gate"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/flow_gate.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptFlowGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowGateData& data) const override;
	};


	class ScriptFlowOnceData : public ScriptStateData<ScriptFlowOnceData> {
	public:
		ScriptFlowOnceData() = default;
		ScriptFlowOnceData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<bool> active{};
	};

	class ScriptFlowOnce : public ScriptNodeTypeBase<ScriptFlowOnceData> {
	public:
		String getId() const override { return "flowOnce"; }
		String getName() const override { return "Flow Once"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/flow_once.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptFlowOnceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFlowOnceData& data) const override;
	};


	class ScriptLatchData final : public ScriptStateData<ScriptLatchData> {
	public:
		bool latched = false;
		ConfigNode value;

		ScriptLatchData() = default;
		ScriptLatchData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptLatch final : public ScriptNodeTypeBase<ScriptLatchData> {
	public:
		String getId() const override { return "latch"; }
		String getName() const override { return "Latch"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/latch.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptLatchData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLatchData& data) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLatchData& curData) const override;
	};


	class ScriptFenceData final : public ScriptStateData<ScriptFenceData> {
	public:
		bool signaled = false;

		ScriptFenceData() = default;
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptFence final : public ScriptNodeTypeBase<ScriptFenceData> {
	public:
		String getId() const override { return "fence"; }
		String getName() const override { return "Fence"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/fence.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptFenceData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptFenceData& curData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptFenceData& curData) const override;
	};


	class ScriptBreakerData final : public ScriptStateData<ScriptBreakerData> {
	public:
		bool signaled = false;
		bool active = false;

		ScriptBreakerData() = default;
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptBreaker final : public ScriptNodeTypeBase<ScriptBreakerData> {
	public:
		String getId() const override { return "breaker"; }
		String getName() const override { return "Breaker"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/breaker.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptBreakerData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptBreakerData& curData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptBreakerData& curData) const override;
	};


	class ScriptSignal final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "signal"; }
		String getName() const override { return "Signal"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/signal.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	
	class ScriptLineResetData final : public ScriptStateData<ScriptLineResetData> {
	public:
		bool active = false;
		bool signaled = false;
		ConfigNode monitorVariable;

		ScriptLineResetData() = default;
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptLineReset final : public ScriptNodeTypeBase<ScriptLineResetData> {
	public:
		String getId() const override { return "lineReset"; }
		String getName() const override { return "Line Reset"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/line_reset.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptLineResetData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data, ScriptLineResetData& curData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLineResetData& curData) const override;
	};
}
