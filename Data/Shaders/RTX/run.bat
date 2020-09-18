
:: Raygen
C:/VulkanSDK/Bin32/glslangValidator.exe -V RayGen/RayGen.rgen -o build/RayGen.raygen

:: Miss Shaders
C:/VulkanSDK/Bin32/glslangValidator.exe -V Miss/GradientMiss.rmiss -o build/GradientMiss.miss


:: Hit Shaders
	:: Textured No Light
C:/VulkanSDK/Bin32/glslangValidator.exe -V Opaque/TexturedNoLight.rchit -o build/TexturedNoLight.hit
	:: Textured Reflective
C:/VulkanSDK/Bin32/glslangValidator.exe -V Opaque/TexturedReflective.rchit -o build/TexturedReflective.hit


pause