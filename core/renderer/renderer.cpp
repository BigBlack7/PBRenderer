#include "renderer.hpp"
#include "thread/threadPool.hpp"
#include "utils/progress.hpp"

namespace pbrt
{
    // 渐进式渲染
    void Renderer::Render(const std::filesystem::path &filename, size_t spp)
    {
        size_t current_spp = 0, increase = 1;
        auto &film = mCamera.GetFilm();
        film.Clear();
        Progress progress(film.GetWidth() * film.GetHeight() * spp);
        while (current_spp < spp)
        {
            MasterThreadPool.ParallelFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
                                         {
                                             for (int i = 0; i < increase; i++)
                                             {
                                                 film.AddSample(x, y, RenderPixel({x, y, current_spp + i}));
                                             }
                                             progress.Update(increase);
                                             // end
                                         });
            MasterThreadPool.Wait();
            current_spp += increase;
            increase = std::min<size_t>(current_spp, 32);
            film.Save(filename);
        }
    }
}