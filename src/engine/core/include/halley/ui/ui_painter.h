#pragma once
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class TextRenderer;
	class Sprite;
	class SpritePainter;
	class Painter;

	class UIPainter {
	public:
		UIPainter(SpritePainter& painter, int mask, int layer);
		UIPainter(UIPainter&& other) noexcept = default;

		UIPainter& operator=(UIPainter&& other) noexcept = default;

		void draw(const Sprite& sprite, bool forceCopy = false);
		void draw(const TextRenderer& text, bool forceCopy = false);
		void draw(Sprite&& sprite);
		void draw(TextRenderer&& text);
		void draw(std::function<void(Painter&)> f);

		UIPainter clone() const;
		UIPainter withAdjustedLayer(int delta) const;
		UIPainter withClip(std::optional<Rect4f> clip) const;
		UIPainter withMask(int mask) const;
		UIPainter withNoClip() const;
		UIPainter withAlpha(float alpha) const;

		std::optional<Rect4f> getClip() const;
		int getMask() const;

	private:
		SpritePainter* painter = nullptr;
		std::optional<Rect4f> clip;
		int mask;
		int layer;
		std::optional<float> alphaMultiplier;
		mutable int currentPriority = 0;
		const UIPainter* rootPainter = nullptr;

		float getCurrentPriorityAndIncrement() const;

		void applyAlpha(TextRenderer& text) const;
		void applyAlpha(Sprite& sprite) const;
	};
}
