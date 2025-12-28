# IdealPlace

Find the ideal location on the Earth for you

![Sample output](sample2.png?raw=true "Sample output")

## Build and run

Should be as easy as:

	make
	./idealplace

The program will run and output a 3600x1800 pixel 16-bit greyscale PNG of the Earth, with lighter colors representing areas which match the given preferences.

On OSX, first install Homebrew, then install the two dependencies:

	brew install libpng pkg-config
	make
	./idealplace


## Options

All options can be preceded by either a "+" or a "-" and they will all be weighted similarly. To apply different weights to the different criteria, either preceed each argument with "-" for less weight or "+" for more weight, and use more of each character to halve or double the weight.

	-boston				Set the preferences to Boston, USA
	-tf januarylow januaryhigh julylow julyhigh		(all in F)
	-tc januarylow januaryhigh julylow julyhigh		(all in C)
	-wtf januarylow januaryhigh 	(all in F)
	-wtc januarylow januaryhigh 	(all in C)
	-stf julylow julyhigh			(all in F)
	-stc julylow julyhigh			(all in C)
	-m [1-12]			Evaluate only for a specific month
	-mtf low high			(low and high temps for that month in F)
	-mtc low high			(low and high temps for that month in C)
	-mr value			Average precipitation in mm/month
	-ac value			Average cloud cover (0 to 1)
	-wmph value			Average wind speed (mph)
	-wmps value			Average wind speed (m/s)
	-hdi value			Human development index (0 to 1)
	-mtn value			Proximity to and magnitude of terrain (0 to 1)
	-ct lat lon			Close to a given location (N lat and E lon, use negative for S and W)
	-ff lat lon			Far from a given location (N lat and E lon, use negative for S and W)
	-new				Start setting preferences for a second person
	-o name.png			Output file name

For example, to select for only annual rainfall and wind, but have rainfall be twice as "important" as wind, use any of these:

	./idealplace ++mr 100 +wmph 8
	./idealplace +mr 100 -wmph 8
	./idealplace -mr 100 --wmph 8

You can use up to 50 "+" or "-", but at that point, just remove all other criteria arguments from the command line, or just look at the source png image for your ideal place.

## Sources

* Air temperature and precipitation from the ssp245 (most-likely scenario) projection from [GloH2O](https://www.gloh2o.org/koppen/) dataset.
* Clouds are a 2021-4 average, data from [MERRA-2](https://disc.gsfc.nasa.gov/datasets/M2TMNXRAD_5.12.4/summary).
* Elevation from [GDEMM2024](https://dataservices.gfz-potsdam.de/panmetaworks/showshort.php?id=b2d17f8d-f599-11ee-967a-4ffbfe06208e), further processed to determine per-pixel elevation variance.
* Human Development Index from [Mosaiks](https://www.mosaiks.org/hdi); I expanded it from -56:74 to -90:90 and filled in the holes.
* Wind speed from [GWA3](https://globalwindatlas.info/download/gis-files)
* Population density from [Copernicus](https://human-settlement.emergency.copernicus.eu/download.php?ds=pop).
* National boundary lines from [naturalearthdata.org](http://naturalearthdata.org/).
* While I don't use it in the code, I enjoy browsing [WeatherSpark](https://weatherspark.com/).

## To Do

* Use the open-water data set to make an image of distance-to-water (to allow "more coastal" option)
* And use that to mask off all open water areas (like the Caspian Sea)
* Generate a layer with US state boundaries to aid in locating these places, make that optional
* Support optional "no elevation less than x" or a desired elevation (use log scale?)
* Support argument "prefer weather like lat-lon" and "prefer all settings like lat-lon"
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
