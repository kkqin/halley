#include <halley/support/exception.h>
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/shader.h"
#include "api/video_api.h"
#include "halley/utils/hash.h"

using namespace Halley;

static const Material* currentMaterial = nullptr;
static int currentPass = 0;
static uint64_t currentHash = 0;

constexpr static int shaderStageCount = int(ShaderType::NumOfShaderTypes);

MaterialDataBlock::MaterialDataBlock()
{
}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlockType type, size_t size, int bindPoint, std::string_view name, const MaterialDefinition& def)
	: data(type == MaterialDataBlockType::SharedExternal ? 0 : size, 0)
	, addresses(def.getNumPasses() * shaderStageCount)
	, dataBlockType(type)
	, bindPoint(static_cast<int16_t>(bindPoint))
{
	for (int i = 0; i < def.getNumPasses(); ++i) {
		auto& shader = def.getPass(i).getShader();
		for (int j = 0; j < shaderStageCount; ++j) {
			addresses[i * shaderStageCount + j] = shader.getBlockLocation(name, ShaderType(j));
		}
	}
}

MaterialDataBlock::MaterialDataBlock(const MaterialDataBlock& other)
	: data(other.data)
	, addresses(other.addresses)
	, dataBlockType(other.dataBlockType)
	, needToUpdateHash(other.needToUpdateHash)
	, bindPoint(other.bindPoint)
	, hash(other.hash)
{}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlock&& other) noexcept
	: data(std::move(other.data))
	, addresses(std::move(other.addresses))
	, dataBlockType(other.dataBlockType)
	, needToUpdateHash(other.needToUpdateHash)
	, bindPoint(other.bindPoint)
	, hash(other.hash)
{
	other.hash = 0;
}

int MaterialDataBlock::getAddress(int pass, ShaderType stage) const
{
	return addresses[pass * shaderStageCount + int(stage)];
}

int MaterialDataBlock::getBindPoint() const
{
	return bindPoint;
}

gsl::span<const gsl::byte> MaterialDataBlock::getData() const
{
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

MaterialDataBlockType MaterialDataBlock::getType() const
{
	return dataBlockType;
}

uint64_t MaterialDataBlock::getHash() const
{
	if (needToUpdateHash) {
		needToUpdateHash = false;
		Hash::Hasher hasher;
		hasher.feedBytes(getData());
		hash = hasher.digest();
	}
	return hash;
}

bool MaterialDataBlock::operator==(const MaterialDataBlock& other) const
{
	return getData() == other.getData();
}

bool MaterialDataBlock::operator!=(const MaterialDataBlock& other) const
{
	return getData() != other.getData();
}

bool MaterialDataBlock::setUniform(size_t offset, ShaderParameterType type, const void* srcData)
{
	Expects(dataBlockType != MaterialDataBlockType::SharedExternal);

	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= data.size());
	Expects(offset % 4 == 0); // Alignment

	if (memcmp(data.data() + offset, srcData, size) != 0) {
		memcpy(data.data() + offset, srcData, size);
		needToUpdateHash = true;
		return true;
	} else {
		return false;
	}
}

Material::Material(const Material& other)
	: materialDefinition(other.materialDefinition)
	, uniforms(other.uniforms)
	, textureUniforms(other.textureUniforms)
	, dataBlocks(other.dataBlocks)
	, textures(other.textures)
	, passEnabled(other.passEnabled)
	, stencilReferenceOverride(other.stencilReferenceOverride)
{
	for (auto& u: uniforms) {
		u.rebind(*this);
	}
}

Material::Material(Material&& other) noexcept
	: materialDefinition(std::move(other.materialDefinition))
	, uniforms(std::move(other.uniforms))
	, textureUniforms(std::move(other.textureUniforms))
	, dataBlocks(std::move(other.dataBlocks))
	, textures(std::move(other.textures))
	, passEnabled(other.passEnabled)
	, stencilReferenceOverride(other.stencilReferenceOverride)
{
	for (auto& u: uniforms) {
		u.rebind(*this);
	}
	other.fullHashValue = 0;
	other.partialHashValue = 0;
}

Material::Material(std::shared_ptr<const MaterialDefinition> definition, bool forceLocalBlocks)
	: materialDefinition(std::move(definition))
{
	const size_t numPasses = materialDefinition->getNumPasses();
	if (numPasses > passEnabled.size()) {
		throw Exception("Too many passes in material.", HalleyExceptions::Graphics);
	}

	for (size_t i = 0; i < passEnabled.size(); ++i) {
		if (i < numPasses) {
			passEnabled[i] = materialDefinition->getPass(static_cast<int>(i)).isEnabled();
		} else {
			passEnabled[i] = false;
		}
	}
	
	initUniforms(forceLocalBlocks);
}

void Material::initUniforms(bool forceLocalBlocks)
{
	int blockNumber = 0;
	int nextBindPoint = 1;
	for (auto& uniformBlock : materialDefinition->getUniformBlocks()) {
		size_t curOffset = 0;
		for (auto& uniform: uniformBlock.uniforms) {
			auto size = MaterialAttribute::getAttributeSize(uniform.type);
			curOffset = alignUp(curOffset, std::min(size_t(16), size));
			uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type, blockNumber, curOffset));
			curOffset += size;
		}
		auto type = uniformBlock.name == "HalleyBlock"
			? (forceLocalBlocks ? MaterialDataBlockType::SharedLocal : MaterialDataBlockType::SharedExternal)
			: MaterialDataBlockType::Local;
		int bind = type == MaterialDataBlockType::Local ? nextBindPoint++ : 0;
		dataBlocks.push_back(MaterialDataBlock(type, curOffset, bind, uniformBlock.name, *materialDefinition));
		++blockNumber;
	}

	// Load textures
	const auto& textureDefs = materialDefinition->getTextures();
	const size_t nTextures = textureDefs.size();
	textures.reserve(nTextures);
	textureUniforms.reserve(nTextures);
	for (size_t i = 0; i < nTextures; ++i) {
		const auto& tex = textureDefs[i];
		textureUniforms.push_back(MaterialTextureParameter(*this, tex.name, tex.samplerType));
		textures.push_back(tex.defaultTexture);
	}
}

void Material::bind(int passNumber, Painter& painter) const
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == passNumber && currentHash == getFullHash()) {
		return;
	}
	currentMaterial = this;
	currentPass = passNumber;
	currentHash = getFullHash();

	painter.setMaterialPass(*this, passNumber);
}

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
	currentHash = 0;
}

bool Material::operator==(const Material& other) const
{
	// Same instance
	if (this == &other) {
		return true;
	}

	constexpr bool useHash = true;

	if (useHash) {
		// If the hashes match, we'll assume they're the same
		// There's a chance this will fail, but... what are the odds? :D
		// :D :D
		// :D :D :D
		return getFullHash() == other.getFullHash();
	} else {
		// Different definitions
		if (materialDefinition != other.materialDefinition) {
			return false;
		}
		
		// Different textures (only need to check pointer equality)
		if (textures != other.textures) {
			return false;
		}

		// Different data
		if (dataBlocks != other.dataBlocks) {
			return false;
		}

		// Different passes enabled
		if (passEnabled != other.passEnabled) {
			return false;
		}

		// Must be the same
		return true;
	}
}

bool Material::operator!=(const Material& material) const
{
	return !(*this == material);
}

bool Material::isCompatibleWith(const Material& other) const
{
	if (materialDefinition != other.materialDefinition) {
		return false;
	}

	if (textures != other.textures) {
		return false;
	}

	return true;
}

const Vector<std::shared_ptr<const Texture>>& Material::getTextures() const
{
	return textures;
}

size_t Material::getNumTextureUnits() const
{
	return textures.size();
}

bool Material::setUniform(int blockNumber, size_t offset, ShaderParameterType type, const void* data)
{
	if (dataBlocks[blockNumber].setUniform(offset, type, data)) {
		needToUpdateHash = true;
		return true;
	}
	return false;
}

void Material::computeHashes() const
{
	Hash::Hasher hasher;

	hasher.feed(materialDefinition.get());
	
	for (const auto& dataBlock: dataBlocks) {
		hasher.feedBytes(dataBlock.getData());
	}

	hasher.feed(stencilReferenceOverride.has_value());
	hasher.feed(stencilReferenceOverride.value_or(0));
	hasher.feed(passEnabled.to_ulong());

	partialHashValue = hasher.digest();

	for (const auto& texture: textures) {
		hasher.feed(texture.get());
	}

	fullHashValue = hasher.digest();
}

const std::shared_ptr<const Texture>& Material::getFallbackTexture() const
{
	return materialDefinition->getFallbackTexture();
}

const std::shared_ptr<const Texture>& Material::getTexture(int textureUnit) const
{
	auto& tex = textureUnit >= 0 && textureUnit < int(textures.size()) ? textures[textureUnit] : getFallbackTexture();
	if (!tex) {
		return getFallbackTexture();
	}
	return tex;
}

std::shared_ptr<const Texture> Material::getRawTexture(int textureUnit) const
{
	return textureUnit >= 0 && textureUnit < int(textures.size()) ? textures[textureUnit] : std::shared_ptr<const Texture>();
}

const Vector<MaterialParameter>& Material::getUniforms() const
{
	return uniforms;
}

const Vector<MaterialDataBlock>& Material::getDataBlocks() const
{
	return dataBlocks;
}

void Material::setPassEnabled(int pass, bool enabled)
{
	if (passEnabled[pass] != enabled) {
		needToUpdateHash = true;
		passEnabled[pass] = enabled;
	}
}

bool Material::isPassEnabled(int pass) const
{
	const size_t p = static_cast<size_t>(pass);
	if (p > passEnabled.size()) {
		return false;
	}
	return passEnabled[p];
}

MaterialDepthStencil Material::getDepthStencil(int pass) const
{
	if (stencilReferenceOverride) {
		auto depthStencil = getDefinition().getPass(pass).getDepthStencil();
		depthStencil.setStencilReference(stencilReferenceOverride.value());
		return depthStencil;
	} else {
		return getDefinition().getPass(pass).getDepthStencil();
	}
}

void Material::setStencilReferenceOverride(std::optional<uint8_t> reference)
{
	if (stencilReferenceOverride != reference) {
		stencilReferenceOverride = reference;
		needToUpdateHash = true;
	}
}

std::optional<uint8_t> Material::getStencilReferenceOverride() const
{
	return stencilReferenceOverride;
}

const Vector<MaterialTextureParameter>& Material::getTextureUniforms() const
{
	return textureUniforms;
}

Material& Material::set(std::string_view name, const std::shared_ptr<const Texture>& texture)
{
	const auto& texs = materialDefinition->getTextures();
	for (size_t i = 0; i < texs.size(); ++i) {
		if (texs[i].name == name) {
			const auto textureUnit = i;
			if (textures[textureUnit] != texture) {
				textures[textureUnit] = texture;
				needToUpdateHash = true;
			}
			return *this;
		}
	}

	throw Exception("Texture sampler \"" + String(name) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

Material& Material::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	return set(name, std::shared_ptr<const Texture>(texture));
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	const auto& texs = materialDefinition->getTextures();
	if (textureUnit < texs.size()) {
		if (textures[textureUnit] != texture) {
			textures[textureUnit] = texture;
			needToUpdateHash = true;
		}
		return *this;
	}

	throw Exception("Texture unit \"" + toString(textureUnit) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	return set(textureUnit, std::shared_ptr<const Texture>(texture));
}

bool Material::hasParameter(std::string_view name) const
{
	for (auto& u: uniforms) {
		if (u.name == name) {
			return true;
		}
	}
	return false;
}

uint64_t Material::getPartialHash() const
{
	if (needToUpdateHash) {
		computeHashes();
		needToUpdateHash = false;
	}
	return partialHashValue;
}

uint64_t Material::getFullHash() const
{
	if (needToUpdateHash) {
		computeHashes();
		needToUpdateHash = false;
	}
	return fullHashValue;
}

MaterialParameter& Material::getParameter(std::string_view name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}

	throw Exception("Uniform \"" + String(name) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

void Material::setDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	materialDefinition = std::move(definition);
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}


MaterialUpdater::MaterialUpdater(std::shared_ptr<const Material>& orig)
	: orig(&orig)
	, material(orig->clone())
{
}

MaterialUpdater::MaterialUpdater(MaterialUpdater&& other) noexcept
{
	orig = other.orig;
	other.orig = nullptr;
	material = std::move(other.material);
}

MaterialUpdater::~MaterialUpdater()
{
	if (orig) {
		*orig = material;
	}
}

MaterialUpdater& MaterialUpdater::operator=(MaterialUpdater&& other) noexcept
{
	if (this != &other) {
		orig = other.orig;
		other.orig = nullptr;
		material = std::move(other.material);
	}
	return *this;
}

bool MaterialUpdater::isValid() const
{
	return orig != nullptr;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const std::shared_ptr<const Texture>& texture)
{
	material->set(name, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	material->set(name, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	material->set(textureUnit, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	material->set(textureUnit, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::setPassEnabled(int pass, bool enabled)
{
	material->setPassEnabled(pass, enabled);
	return *this;
}

MaterialUpdater& MaterialUpdater::setStencilReferenceOverride(std::optional<uint8_t> reference)
{
	material->setStencilReferenceOverride(reference);
	return *this;
}

MaterialParameter& MaterialUpdater::getParameter(std::string_view name)
{
	return material->getParameter(name);
}
