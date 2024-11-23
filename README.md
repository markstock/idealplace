# IdealPlace

Find the ideal location on the Earth for you

## Build and run

Should be as easy as

	make

	./idealplace

## Options

None yet.

## Sources

* Air temperature and precipitation from the ssp245 (most-likely scenario) projection from [GloH2O](https://www.gloh2o.org/koppen/) dataset.
* Clouds are a 2023 average, data from [MERRA-2](https://disc.gsfc.nasa.gov/datasets/M2TMNXRAD_5.12.4/summary).
* Elevation from [GDEMM2024](https://dataservices.gfz-potsdam.de/panmetaworks/showshort.php?id=b2d17f8d-f599-11ee-967a-4ffbfe06208e), further processed to determine per-pixel elevation variance.
* Population density from [Copernicus](https://human-settlement.emergency.copernicus.eu/download.php?ds=pop).
* Human Development Index from [Mosaiks](https://www.mosaiks.org/hdi); I expanded it from -56:74 to -90:90 and filled in the holes.
* While I don't use it in the code, I enjoy browsing [WeatherSpark](https://weatherspark.com/).

## To Do

* Use the open-water data set to make an image of distance-to-water
* And use that to mask off all open water areas (like the Caspian Sea)
* Generate a layer with national and US state boundaries to aid in locating these places

