# GitHub Actions build

1. Push this whole folder to a GitHub repository with GitHub Desktop.
2. Open **Actions** in the repository.
3. Choose **Build Nintendo Game Music Viewer V1 (Windows x64)**.
4. Start the workflow on `main`.
5. When it succeeds, download the Windows ZIP from **Artifacts**.

No .NET SDK is required. The workflow uses CMake/Ninja on a Windows runner and fetches SDL2, SDL2_ttf and libgme during the build.


### CMake compatibility fix
The GitHub Actions workflow passes `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` so the vendored FreeType copy used by SDL2_ttf 2.24.0 can configure under current CMake versions.
