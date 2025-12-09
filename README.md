# YAPT
Yet another path tracer, influenced by Peter Shirley's Raytracing in one weekend series

## Usage:
```
yapt width=250 spp=100 aggregator=vor sampler=sppp maxdepth=10 nee=true source=/path/to/scene/some_scene.ypt

qtvor width=250 spp=100 aggregator=vor sampler=sppp maxdepth=10 nee=true source=/path/to/scene/some_scene.ypt
```


## Requirements:
- Libtorch 2.8.0+cpu installed in /opt/libtorch
- Intel MKL 2025.2.0 installed in /opt/intel/oneapi/mkl/2025.2