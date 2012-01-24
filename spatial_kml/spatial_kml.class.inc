<?php

/**
 * Could also look at parsing shapefiles manually: http://en.wikipedia.org/wiki/Shapefile#Shapefile_shape_format_.28.shp.29
 * Could also implement Iterator: http://php.net/manual/en/class.iterator.php
 */
class SpatialKML {
  var $uri = '';

  function __construct($uri) {
    // TODO: Check to make sure this file exists
    $this->uri = drupal_realpath($uri);
  }

  function process() {
    $result = array();

    if ($ogr2ogr_kml = ogr2ogr_open($this->uri)) {
      if ($spatial_features = $ogr2ogr_kml->getWkt()) {
//        print_r($spatial_features);exit(2);
        $result = $spatial_features;
      }
    }

    return $result;
  }

}
