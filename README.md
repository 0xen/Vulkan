### This project is under development and will see changes

# Vulkan SDK Examples and Demos
The idea behind this project is to create a comprehensive guide to help developers new to Vulkan, understand all its components fully. To do this I have broken down each section into an example project showcasing how that area of Vulkan is managed in its simplest forum.

## Table of Contents
+ [Building](#Building)
+ [Getting Started](#GettingStarted)



## <a name="Building"></a> Building
### Windows
The project is created using CMake and will allow you to build the example executables or as a Visual Studio Solution

#### Build Visual Studio Solution

```cmake -H. -Bbuild --log-level=NOTICE -G "Visual Studio 16 2019" -A x64```

### Linux
Coming Soon

## <a name="GettingStarted"></a> Getting Started

### [0 - Instance](0-GettingStarted/0-Instance/)
How to create the underline 'VkInstance' with basic layer validation. 

### [1 - Physical Device](0-GettingStarted/1-PhysicalDevice/)
Find a device on the system that is compatible with our requirements for the system. There could be multiple GPUs within the computer and I show several ways of distinguishing the best choice for the project.

### [2 - Device](0-GettingStarted/2-Device/)
Upon finding the GPU that we want to use, we next create a Vulkan Device instance.

### [3 - Command Pool](0-GettingStarted/3-CommandPools/)
Creating the mechanism that we will use to send messages to the GPU.

### [4 - Buffers](0-GettingStarted/4-Buffers/)
We create a buffer on the GPU, allocate and bind memory to it, and show examples of reading and writing to and from it.

### [5 - Descriptors](0-GettingStarted/5-Descriptors/)
Create a descriptor pool and descriptor set that later on will allow for the dynamic binding of resources to a pipeline

### [6 - Swapchain](0-GettingStarted/6-Swapchain/)
Here we create the swapchain and all of its images for later rendering steps

### [7 - Renderpass](0-GettingStarted/7-Renderpass/)
Create the render pass, this involves defining what the rendering order will be, how many post-processes will there be, how many image attachments will be needed, etc

### [8 - Present](0-GettingStarted/8-Present/)
Render a blank screen with a single color, here we focus on getting the rendering commands created and render a basic screen using them

### [9 - GraphicsPipeline](0-GettingStarted/9-GraphicsPipeline/)
Create a basic graphics pipeline and render a triangle. Here we load a SPIRE-V Vertex and Fragment shader as well as walk through the various settings involved in the creation of a graphics pipeline

### [10 - Texturing](0-GettingStarted/10-Texturing/)
Following on from the graphics pipeline tutorial, we create a simple quad and render a basic texture to it. Here we show loading an image from a file using lodepng, creating a sampler for it and rendering it

### [11 - Camera](0-GettingStarted/11-Camera/)
Here we create a basic camera with a view and projection matrix. During this tutorial, we introduce two new helpful libraries that can help, GLM for vector and matrix maths and objloader to allow us to import more complex models easier

## <a name="Advanced"></a> Advanced

### [0 - IndirectDrawing](1-Advanced/0-IndirectDrawing/)
Indirect Drawing allows us to dynamically update what we are rendering and how many instances of a model we are rendering without rebuilding the command buffers. In this example, we create an array of model positions and dynamically tell the GPU how many models we want to render

### [1 - ComputePipeline](1-Advanced/1-ComputePipeline/)
Compute pipelines are very useful for a whole range of tasks within rendering, it can be used for many tasks that can be highly parallelized, such as a particle effect system. In this example, we show you a stripped-down version of the renderer that exposes what goes on with the compute pipeline. We create an array of floating-point numbers and get the GPU to multiply them by a fixed amount. Future examples will expand on this

### [2 - ComputeParticleSystem](1-Advanced/2-ComputeParticleSystem/)
In this example, we create a particle system. This particle system takes advantage of compute shaders from the previous project and allows for the GPU to update the positions and color of the particles. The benefit of using a GPU based particle system over a CPU based on is after every update of a CPU based particle system, we have to transfer the data over to the GPU. This is fine with small particle systems, but if we end up dealing with thousands of systems each with thousands of particles, it will end up causing a bottleneck. Instead, if we use GPU updated particles, each particle gets updated highly multithreaded on the GPU with no need to transfer data between the CPU and GPU memory.

### [3 - AccelerationStructure](1-Advanced/3-AccelerationStructure/)
In this example we cover how to prepair your model to be used with RTX by creating it a Acceleration Structure, after this we take the model acceleration structure or "bottom level AS" and make a top level acceleration structure that will be used for triangle ray intersection calculations in the next tutorial

### [4 - RaytracingPipeline](1-Advanced/4-RaytracingPipeline/)
In this example we cover how to create a raytracing pipeline, how to render using it in the command buffer recording and finaly some examples of shaders needed to get RTX up and running