# IdealPlace

Find the ideal location on the Earth for you

![Sample output](sample_lowres.png?raw=true "Sample output")

## Build and run

Should be as easy as

	make

	./idealplace

The program will run and output a 3600x1800 pixel 16-bit greyscale PNG of the Earth, with lighter colors representing areas which match the given preferences.

## Options

	-boston				Set the preferences to Boston, USA
	-tf januarylow januaryhigh julylow julyhigh		(all in F)
	-tc januarylow januaryhigh julylow julyhigh		(all in C)
	-mr value			Average precipitation in mm/month
	-ac value			Average cloud cover (0 to 1)
	-wmph value			Average wind speed (mph)
	-wmps value			Average wind speed (m/s)
	-hdi value			Human development index (0 to 1)
	-mtn value			Proximity to terrain (0 to 1)
	-new				Start setting preferences for a second person
	-o name.png			Output file name

## Sources

* Air temperature and precipitation from the ssp245 (most-likely scenario) projection from [GloH2O](https://www.gloh2o.org/koppen/) dataset.
* Clouds are a 2021-4 average, data from [MERRA-2](https://disc.gsfc.nasa.gov/datasets/M2TMNXRAD_5.12.4/summary).
* Elevation from [GDEMM2024](https://dataservices.gfz-potsdam.de/panmetaworks/showshort.php?id=b2d17f8d-f599-11ee-967a-4ffbfe06208e), further processed to determine per-pixel elevation variance.
* Human Development Index from [Mosaiks](https://www.mosaiks.org/hdi); I expanded it from -56:74 to -90:90 and filled in the holes.
* Wind speed from [GWA3](https://globalwindatlas.info/download/gis-files)
* Population density from [Copernicus](https://human-settlement.emergency.copernicus.eu/download.php?ds=pop).
* While I don't use it in the code, I enjoy browsing [WeatherSpark](https://weatherspark.com/).

## To Do

* Use the open-water data set to make an image of distance-to-water (to allow "more coastal" option)
* And use that to mask off all open water areas (like the Caspian Sea)
* Generate a layer with national and US state boundaries to aid in locating these places
* Support optional "no elevation less than x" or a desired elevation (use log scale?)
* Optional argument "prefer far away from lat-lon" or "prefer close to lat-lon"
* Support argument "prefer weather like lat-lon" and "prefer all settings like lat-lon"
* Even make temperature optional, or add a weight to temperature to allow ignoring it
* Use population density to allow "in a city but near the country" or "in the country but near a city"
* Find a higher-resolution cloud data set, if possible

## Citing IdealPlace

I don't get paid for writing or maintaining this, so if you find this tool useful or mention it in your writing, please cite it by using the following BibTeX entry.

```
@Misc{IdealPlace2024,
  author =       {Mark J.~Stock},
  title =        {Ideal{P}lace:  {F}ind the ideal location on the {E}arth for you},
  howpublished = {\url{https://github.com/markstock/idealplace}},
  year =         {2024}
}
```
