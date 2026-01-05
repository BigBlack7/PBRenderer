// Example: How to use Sobol Sampler in your renderer
// 示例：如何在渲染器中使用 Sobol 采样器

#include "sampler/sobolSampler.hpp"
#include "sampler/rngSampler.hpp"
#include <iostream>

namespace pbrt
{
    // Example 1: Basic usage in a render loop
    // 示例 1：在渲染循环中的基本使用
    void ExampleBasicUsage()
    {
        SobolSampler sampler;
        
        // Render a 10x10 image with 4 samples per pixel
        for (int y = 0; y < 10; y++)
        {
            for (int x = 0; x < 10; x++)
            {
                for (int sample = 0; sample < 4; sample++)
                {
                    // Initialize sampler for this pixel and sample
                    sampler.StartPixelSample(glm::ivec2(x, y), sample);
                    
                    // Get 2D sample for camera ray
                    glm::vec2 camera_sample = sampler.Get2D();
                    
                    // Generate ray (pseudo-code)
                    // Ray ray = camera.GenerateRay(x, y, camera_sample);
                    
                    // Path tracing loop
                    for (int depth = 0; depth < 5; depth++)
                    {
                        // Get 2D sample for BSDF
                        glm::vec2 bsdf_sample = sampler.Get2D();
                        
                        // Russian roulette
                        float rr_sample = sampler.Get1D();
                        if (rr_sample > 0.95f) break;
                        
                        // Continue tracing...
                    }
                }
            }
        }
    }

    // Example 2: Comparing RNG vs Sobol distribution
    // 示例 2：比较 RNG 与 Sobol 的分布
    void ExampleCompareDistribution()
    {
        std::cout << "Comparing sample distributions:\n\n";
        
        // RNG samples
        std::cout << "RNG Samples (first 10):\n";
        RNGSampler rng_sampler(12345);
        rng_sampler.StartPixelSample(glm::ivec2(0, 0), 0);
        for (int i = 0; i < 10; i++)
        {
            glm::vec2 sample = rng_sampler.Get2D();
            std::cout << "  (" << sample.x << ", " << sample.y << ")\n";
        }
        
        std::cout << "\nSobol Samples (first 10):\n";
        SobolSampler sobol_sampler;
        sobol_sampler.StartPixelSample(glm::ivec2(0, 0), 0);
        for (int i = 0; i < 10; i++)
        {
            glm::vec2 sample = sobol_sampler.Get2D();
            std::cout << "  (" << sample.x << ", " << sample.y << ")\n";
        }
        
        std::cout << "\nNotice: Sobol samples are more evenly distributed!\n";
    }

    // Example 3: Thread-safe usage
    // 示例 3：线程安全使用
    void ExampleThreadSafe()
    {
        // Each thread gets its own sampler instance
        #pragma omp parallel for
        for (int pixel_id = 0; pixel_id < 100; pixel_id++)
        {
            // Thread-local sampler
            thread_local SobolSampler sampler;
            
            int x = pixel_id % 10;
            int y = pixel_id / 10;
            
            sampler.StartPixelSample(glm::ivec2(x, y), 0);
            
            // Use sampler for this pixel...
            glm::vec2 sample = sampler.Get2D();
        }
    }

    // Example 4: Migration from RNG to Sobol
    // 示例 4：从 RNG 迁移到 Sobol
    void ExampleMigration()
    {
        // OLD CODE (using RNG):
        /*
        thread_local RNG rng;
        rng.SetSeed(pixel_x + pixel_y * 10000 + sample_index * 10000000);
        float u = rng.Uniform();
        float v = rng.Uniform();
        */
        
        // NEW CODE (using Sobol):
        thread_local SobolSampler sampler;
        int pixel_x = 5, pixel_y = 7, sample_index = 0;
        sampler.StartPixelSample(glm::ivec2(pixel_x, pixel_y), sample_index);
        glm::vec2 uv = sampler.Get2D();
        float u = uv.x;
        float v = uv.y;
        
        std::cout << "Migrated sample: (" << u << ", " << v << ")\n";
    }

    // Example 5: Proper dimension management
    // 示例 5：正确的维度管理
    void ExampleDimensionManagement()
    {
        SobolSampler sampler;
        sampler.StartPixelSample(glm::ivec2(0, 0), 0);
        
        // Dimension 0-1: Camera sample
        glm::vec2 camera_sample = sampler.Get2D();
        
        // Dimension 2-3: First bounce BSDF
        glm::vec2 bsdf_sample_1 = sampler.Get2D();
        
        // Dimension 4: First bounce RR
        float rr_sample_1 = sampler.Get1D();
        
        // Dimension 5-6: Second bounce BSDF
        glm::vec2 bsdf_sample_2 = sampler.Get2D();
        
        // Dimension 7: Second bounce RR
        float rr_sample_2 = sampler.Get1D();
        
        // Total dimensions used: 8
        // Sobol sampler supports up to 32 dimensions
        
        std::cout << "Used 8 dimensions for camera + 2 bounces\n";
    }

    // Run all examples
    void RunAllExamples()
    {
        std::cout << "=== Sobol Sampler Examples ===\n\n";
        
        std::cout << "Example 1: Basic Usage\n";
        ExampleBasicUsage();
        std::cout << "Completed!\n\n";
        
        std::cout << "Example 2: Compare Distribution\n";
        ExampleCompareDistribution();
        std::cout << "\n";
        
        std::cout << "Example 3: Thread-Safe Usage\n";
        ExampleThreadSafe();
        std::cout << "Completed!\n\n";
        
        std::cout << "Example 4: Migration from RNG\n";
        ExampleMigration();
        std::cout << "\n";
        
        std::cout << "Example 5: Dimension Management\n";
        ExampleDimensionManagement();
        std::cout << "\n";
        
        std::cout << "=== All Examples Completed ===\n";
    }
}

// Uncomment to run examples
// int main() {
//     pbrt::RunAllExamples();
//     return 0;
// }
