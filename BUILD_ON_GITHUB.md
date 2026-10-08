# GitHub Actions build

1. Push this whole folder to a GitHub repository with GitHub Desktop.
2. Open **Actions** in the repository.
3. Choose **Build Nintendo Game Music Viewer V1 (Windows x64)**.
4. Start the workflow on `main`.
5. When it succeeds, download the Windows ZIP from **Artifacts**.

No .NET SDK is required. The workflow uses CMake/Ninja on a Windows runner and fetches SDL2, SDL2_ttf and libgme during the build.
