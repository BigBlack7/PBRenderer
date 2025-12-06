#include "renderer.hpp"
#include "thread/threadPool.hpp"
#include "utils/progress.hpp"

namespace pt
{
    void Renderer::Render(const std::filesystem::path &filename, size_t spp)
    {
        size_t current_spp = 0, increase = 1;
        auto &film = mCamera.GetFilm();
        film.Clear();
        Progress progress(film.GetWidth() * film.GetHeight() * spp);
        while(current_spp < spp)
        {
            threadPool.ParallelFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
            {
                for(int i = 0; i < increase; i++)
                {
                    film.AddSample(x, y, RenderPixel({x, y}));
                }
                progress.Update(increase);
            });
            threadPool.Wait();
            current_spp += increase;
            increase = std::min<size_t>(current_spp, 32);
            film.Save(filename);
        }
    }
}