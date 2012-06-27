<?php

/**
 * Could also look at parsing shapefiles manually: http://en.wikipedia.org/wiki/Shapefile#Shapefile_shape_format_.28.shp.29
 * Could also implement Iterator: http://php.net/manual/en/class.iterator.php
 */
class SpatialShapefile {
  var $extracted_path = '';
  var $uri = '';

  function __construct($uri) {
    $this->uri = $uri;
    $this->extracted_path = drupal_realpath('temporary://spatial_shapefile_'. basename($this->uri, '.zip'));

    // Unzip.
    $zip = new ArchiverZip(drupal_realpath($this->uri));
    $zip->extract($this->extracted_path);
  }

  function __destruct() {
    // Delete extracted folder.
    file_unmanaged_delete_recursive($this->extracted_path);
  }

  function process() {
    $result = array();

    // Look for shapefiles.
    $shapefiles = file_scan_directory($this->extracted_path, '/^.*\.(shp)$/');

    foreach ($shapefiles as $shapefile) {
      if ($ogr2ogr_shapefile = ogr2ogr_open($shapefile->uri)) {
        if ($spatial_features = $ogr2ogr_shapefile->getWkt()) {
          $result = array_merge($result, $spatial_features);
        }
      }
    }

    return $result;
  }

}
