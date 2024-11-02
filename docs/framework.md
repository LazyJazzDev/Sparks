# 架构介绍

Sparks Renderer 包含以下几个主要模块：

- `renderer`：渲染器核心模块，包含渲染器的管线创建、渲染过程等。
  - `scene`：场景模块，由 Renderer 创建，包含场景内容的定义。
- `asset manager`：资源管理模块，包含资源的加载、管理、释放等。
- `app`：应用模块，包含应用的初始化、主循环、用户界面等。

## Renderer (渲染器核心)

渲染器核心模块代码位于 [code/sparks/renderer](../code/sparks/renderer) 目录下

Renderer 类包含了所有渲染管线的创建、渲染过程的实现等，其中着色器部分包含了渲染算法的主要实现，为整个渲染器的重中之重。

### Shaders (着色器)

着色器代码位于 [code/sparks/renderer/shaders](../code/sparks/renderer/shaders) 目录下，包含以下几个主要着色器：

- `raytracing.rgen`：光线追踪着色器主文件，包含了光线追踪着色器的入口。
- `raytracing.rchit`：光线追踪着色器的击中着色器，当硬件光追 TraceRay 函数调用击中物体时，会调用此着色器返回击中物体的信息，一般不需要修改。
- `raytracing.rmiss`：光线追踪着色器的未击中着色器，当硬件光追 TraceRay 函数没有击中物体时，会调用此着色器返回未击中情况下的信息，一般不需要修改。
- `entity_pass.frag/vert` & `envmap_pass.frag/vert`： 用于预览场景的着色器。不影响渲染结果，不需要修改。
- `*.glsl`：其他辅助着色器文件，包含了一些辅助函数、结构体定义等。通过 `#include` 引入到其他着色器中。

整个硬件光追的管线大致执行流程如下：

1. 光线生成着色器 (raytracing.rgen) 首先根据屏幕像素位置生成初始光线。
2. 通过 TraceRay 函数追踪光线，调用击中着色器 (raytracing.rchit) 或未击中着色器 (raytracing.rmiss) 返回击中或未击中的信息。
3. 在光线生成着色器中根据击中或未击中的信息计算最终像素颜色。（在一些常见的装追管线中，这一步也可能在击中着色器中完成，但为了保证核心算法的完整性，我们将这一步放在光线生成着色器中完成）

**建议**：从 `raytracing.rgen` 着手，逐步阅读其他着色器文件，了解整个硬件光追的执行流程。

**重要提醒**：在本项目的构建中，着色器文件会在编译阶段被嵌入生成的二进制文件，目前 CMake 的配置文件存在问题导致所有的被引用着色器文件 `*.glsl` 的修改不会触发重新编译，需要手动修改一下你希望更新的主要着色器文件，然后重新编译才能让 `*.glsl` 中的修改生效，如插入或删除一个没有用的空格或回车。

## Asset Manager (资源管理)

资源管理模块代码位于 [code/sparks/asset_manager](../code/sparks/asset_manager) 目录下，包含了资源的加载、管理、释放等。

同时，CPU 端的资源类位于 [code/sparks/assets](../code/sparks/assets) 目录下，包含了一些基础的加载硬盘资源的内容。在导入资源时，需要先加载资源到 CPU 端，然后再将资源上传到 GPU 端。

### Mesh 类

Mesh 类是 CPU 端的三角网格资源类，包含了网格的顶点、索引等信息。

- LoadObjFile 函数：从硬盘加载一个 obj 文件。
- StoreObjFile 函数：将一个 Mesh 以 obj 格式存储到硬盘。

### Texture 类

Texture 类是 CPU 端的纹理资源类，包含了纹理的像素数据、尺寸等信息。

- LoadFromFile 函数：从硬盘加载一个纹理文件。
- StoreToFile 函数：将一个 Texture 以 png 格式存储到硬盘。

### LoadMesh 函数

这是一个 AssetManager 类的成员函数，用于将一个 Mesh 从 CPU 端上传到 GPU 端。返回一个 Mesh ID，用于在场景中引用这个 Mesh。

### LoadTexture

这是一个 AssetManager 类的成员函数，用于将一个 Texture 从 CPU 端上传到 GPU 端。返回一个 Texture ID，用于在场景中引用这个 Texture。

## Scene (场景)

场景文件位于 [code/sparks/scene](../code/sparks/scene) 目录下，包含了场景内容的定义。

两个创建样例场景的函数位于 [code/sparks/app/built_in_scene.h](../code/sparks/app/built_in_scene.h) 文件中。可以用于参考如何创建一个场景。

场景内容包括：

- Entity：实体，用于定义一个场景中可被渲染的物体。
- Envmap：环境贴图，用于环境光照。
- Camera：相机，用于定义渲染的视角。

主要涉及的函数有：

- CreateEntity 函数：创建一个 Entity。
- SetEntityMesh 函数：设置 Entity 的 Mesh。通过 Mesh ID 引用 AssetManager 中的 Mesh。
- SetEntityMaterial 函数：设置 Entity 的 Material。
- SetEntityTransform 函数：设置 Entity 的 Transform。
- SetEntityAlbedoTexture 函数：设置 Entity 的 Albedo 纹理。（用于决定物体基础颜色）
- SetEntityAlbedoDetailTexture 函数：设置 Entity 的 Albedo 细节纹理。（用于决定物体基础颜色的细节）
- SetEntityDetailScaleOffset 函数：设置 Entity 的细节纹理的缩放和偏移。
- SetEnvmapSettings 函数：设置环境贴图。

所有 Set 类函数都拥有对应的 Get 函数，用于获取对应已设置的信息。

### Entity 类

Entity 类用于定义场景中的实体，你不需要直接创建 Entity 类的实例，而是通过 Scene 类的成员函数管理 Entity。

#### Entity Metadata 结构体

Entity Metadata 结构体是 Entity 类的一个成员，包含了实体的 Mesh ID，变换矩阵，各类纹理 ID的引用等信息。定义位于 [code/sparks/scene/entity.h](../code/sparks/scene/entity.h) 文件中。

#### Material 结构体

Material 结构体是 Entity 类的一个成员，包含了纹理类型和主要的材质属性。定义位于 [code/sparks/scene/material.h](../code/sparks/scene/material.h) 文件中。
同时 Material 结构体需要与着色器 [code/sparks/renderer/shaders/material.glsl](../code/sparks/renderer/shaders/material.glsl) 中的材质结构体保持一致，以便在光线追踪着色器中正确地读取材质属性。
现在的材质属性包括了 Principled BSDF 的主要参数，可以通过修改这些参数来调整物体的表面属性。

结构体成员如下：
- Type：材质类型，目前预定义了 Lambertian，Specular 和 Principled BSDF 三种材质代码。
- Base Color：基础颜色，物体的主要颜色。又称反照率（albedo）。
- Subsurface：次表面散射，定义光线进入物体内部进行散射的比例。
- Subsurface Radius：次表面散射半径，三个分量分别定义了红、绿、蓝通道的次表面散射半径。
- Metallic：金属度，定义物体表面的金属性程度。
- Specular：高光强度，定义物体表面的高光程度。
- Specular Tint：高光色调，定义高光的颜色与 Base Color 的贴合程度。
- Roughness：粗糙度，定义物体表面的粗糙程度。
- Anisotropic：各向异性，定义物体表面的各向异性程度。
- Anisotropic Rotation：各向异性旋转，定义各向异性的旋转程度。
- Sheen：光泽，定义物体表面的光泽程度。
- Sheen Tint：光泽色调，定义光泽的颜色。
- Clearcoat：透明面漆，定义物体表面的透明面漆程度。
- Clearcoat Roughness：透明面漆粗糙度，定义透明面漆的粗糙程度。
- IOR：折射率，定义物体的折射率。
- Transmission：透射度，定义物体的透射程度。
- Transmission Roughness：透射粗糙度，定义透射的粗糙程度。
- Emission：自发光，定义物体的自发光颜色。
- Emission Strength：自发光强度，定义物体的自发光强度。
- Alpha：透明度，定义物体的透明度，即光线发生交互的概率。当值为 0 时，物体完全透明，当值为 1 时，物体完全不透明。