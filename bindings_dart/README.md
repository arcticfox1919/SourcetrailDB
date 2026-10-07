# SourcetrailDB Dart binding

`bindings_dart` 是 SourcetrailDB 的 Dart FFI 原生 binding。它提供稳定的 C ABI，把 Dart 索引器的数据写入 SourcetrailDB。

## 构建原则

必须从 SourcetrailDB 根目录构建，不能在 `bindings_dart` 目录中单独执行 CMake：

```text
SourcetrailDB/
├─ CMakeLists.txt          # 顶层构建入口
├─ core/                   # lib_core 静态库
└─ bindings_dart/          # Dart C ABI binding
```

顶层 CMake 会先构建 `lib_core`，再构建 `bindings_dart`。最终 binding 是动态库，内部静态链接 SourcetrailDB 的 `lib_core`：

```text
lib_core       STATIC
bindings_dart  SHARED -> link lib_core
```

这样 Dart FFI 可以通过 `DynamicLibrary.open()` 加载 binding；`lib_core` 本身是静态库，不能直接被 Dart FFI 加载。

## Windows 构建

要求：Visual Studio C++、CMake 和与 Dart 运行时匹配的架构。PowerShell 命令如下：

```powershell
cd G:\Repository\github\SourcetrailDB

cmake -S . -B build `
  -DBUILD_BINDINGS_DART=ON `
  -DBUILD_EXAMPLES=OFF

cmake --build build --config Release --parallel 4
```

构建产物：

```text
build\bindings_dart\Release\sourcetrail_db_bindings.dll
build\bindings_dart\Release\sourcetrail_db_bindings.lib
```

其中 `.dll` 是 Dart FFI 运行时加载的文件，`.lib` 是 Windows 链接阶段生成的导入库，不需要传给 Dart。

只构建 binding target：

```powershell
cmake --build build --config Release --target bindings_dart --parallel 4
```

Debug 构建：

```powershell
cmake --build build --config Debug --target bindings_dart --parallel 4
```

## Linux 和 macOS 构建

```bash
cd /path/to/SourcetrailDB
cmake -S . -B build \
  -DBUILD_BINDINGS_DART=ON \
  -DBUILD_EXAMPLES=OFF
cmake --build build --target bindings_dart --parallel
```

产物通常位于：

```text
Linux:  build/bindings_dart/libsourcetrail_db_bindings.so
macOS:  build/bindings_dart/libsourcetrail_db_bindings.dylib
```

实际文件名以 CMake 输出为准。传给 Dart 的路径必须是动态库文件，而不是 `lib_core` 的静态库。

## 重新配置和清理构建

修改 `bindings_dart` 的 C++ 源码、头文件或顶层 CMake 后，重新配置：

```powershell
cmake -S . -B build -DBUILD_BINDINGS_DART=ON -DBUILD_EXAMPLES=OFF
cmake --build build --config Release --target bindings_dart --parallel 4
```

需要完全清理时，只删除明确的构建目录，然后重新配置：

```powershell
Remove-Item -LiteralPath .\build -Recurse -Force
cmake -S . -B build -DBUILD_BINDINGS_DART=ON -DBUILD_EXAMPLES=OFF
cmake --build build --config Release --target bindings_dart --parallel 4
```

## 与 Dart ffigen 配合

头文件是：

```text
bindings_dart/sourcetrail_db_bindings.h
```

当 C ABI 发生变化时，在 `Sourcetrail/dart_indexer` 中重新生成 Dart 胶水代码：

```powershell
cd G:\Repository\github\Sourcetrail\dart_indexer
dart pub get
dart run ffigen --config ffigen.yaml
```

当前 `ffigen.yaml` 使用：

```yaml
llvm-path:
  - 'D:\develop2\LLVM'
```

生成文件：

```text
dart_indexer/lib/src/native/sourcetrail_db_bindings.dart
```

## Dart 索引器运行时

构建完成后，把 DLL 路径传给 Dart 索引器：

```powershell
cd G:\Repository\github\Sourcetrail\dart_indexer
dart run bin/dart_indexer.dart index `
  --root D:\projects\demo `
  --output-dir D:\indexes\demo `
  --writer-library G:\Repository\github\SourcetrailDB\build\bindings_dart\Release\sourcetrail_db_bindings.dll `
  --database-version 25
```

注意：

- Dart、CMake 和 DLL 必须使用相同的 CPU 架构，例如都使用 x64。
- `--database-version` 必须与 SourcetrailDB 的 `version.txt` 和 Sourcetrail 版本兼容；当前是 `25`。
- Windows 下使用 Release DLL 时，Dart 应用也应使用相匹配的运行库环境。
- 不要把 `bindings_dart` 编译成另一个独立的 SourcetrailDB 副本；它必须链接当前根工程构建出的 `lib_core`。

## 常见错误

### `CORE_BINARY_DIR` 或 `LIB_CORE_TARGET_NAME` 未定义

通常是因为在 `bindings_dart` 目录中直接运行了 CMake。删除该构建目录，回到 SourcetrailDB 根目录重新执行：

```powershell
cmake -S . -B build -DBUILD_BINDINGS_DART=ON -DBUILD_EXAMPLES=OFF
```

### 找不到 `sourcetrail_db_bindings.dll`

确认使用了 `--config Release`，并检查：

```text
build\bindings_dart\Release\sourcetrail_db_bindings.dll
```

### 旧 DLL 仍被加载

确认 `--writer-library` 指向新生成的 `sourcetrail_db_bindings.dll`，并关闭正在使用旧数据库或旧 DLL 的进程后重试。
