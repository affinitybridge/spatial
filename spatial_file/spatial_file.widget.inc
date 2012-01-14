<?php



//define('SPATIAL_FILE_GEOFIELD_GROUP_COLLECTION', 0);
//define('SPATIAL_FILE_GEOFIELD_GROUP_MULTIPLE', 1);
//define('SPATIAL_FILE_GEOFIELD_GROUP_BYTYPE', 2);

/**
 * Implements hook_field_widget_form().
 */
function _spatial_file_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {

  $element += array(
    '#type' => 'markup',
    '#value' => 'This field is being populated from :',
    '#value_callback' => 'shapefile_geofield_widget_value',
  );

  // TODO: Add a form markup item saying that there are no files to select?
  return array(
    'msg' => $element,
  );
}

/**
 * Called from hook_field_widget_settings_form().
 */
function _spatial_file_widget_settings_form($field, $instance) {
  $widget   = $instance['widget'];
  $defaults = field_info_widget_settings($widget['type']);
  $settings = array_merge($defaults, $widget['settings']);

  $form = array();

  if ($widget['type'] == 'spatial_file') {
    $zip_filefields = _spatial_file_widget_get_zip_filefields($field);

    if ($zip_filefields) {

      asort($zip_filefields);
      $form['associated_filefield'] = array(
        '#type'  => 'radios',
        '#title' => t('Associated filefield'),
        '#default_value' => $settings['associated_filefield'],
        '#options' => $zip_filefields,
        '#description' => t('Select the filefield type that is storing the shapefile for this field'),
        '#multiple' => FALSE,
        '#required' => TRUE,
      );
    }
    else {
      // TODO: Display a msg saying no filefields available, reference widget can not be created
    }

    $form['multiple_values'] = array(
      '#type' => 'radios',
      '#title' => t('Multiple value handling'),
      '#options' => array(
        // TODO: Make a hook for these things
        SPATIAL_FILE_GROUPING_COLLECTION => t('Put every feature from each shapefile into a Geometry Collection value.'),
        SPATIAL_FILE_GROUPING_MULTIPLE => t('Put each feature into a its own separate geofield value.'),
        SPATIAL_FILE_GROUPING_BYTYPE => t('Put every feature across all shapefiles into collections by type.'),
      ),
      '#description' => t('Select how multiple values within the shapefile should be handled'),
      '#default_value' => $settings['multiple_values'],
      '#required' => TRUE,
      '#multiple' => FALSE,
    );

    $form['allowed_types'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Multiple value handling'),
      '#description' => t('Select how multiple values within the shapefile should be handled'),
      '#default_value' => $settings['allowed_types'],
      '#options' => array(
        'line' => t('Line'),
        'point' => t('Point'),
        'polygon' => t('Polygon.'),
      ),
      '#multiple' => TRUE,
    );
  }

  return $form;
}


function _spatial_file_widget_get_zip_filefields($field) {
  $filefield_list = array();

  $bundles = array();
  if (isset($field['bundles']['node']) && $field['bundles']['node']) {
    foreach ($field['bundles']['node'] as $bundle) {
      $bundles[] = $bundle;
    }
  }

  // Check all fields
  $field_defs = field_info_fields();
  ksort($field_defs);
  foreach ($field_defs as $field_name => $field_def) {

    // If this is a filefield
    if ($field_def['type'] == 'file' && strcmp($field_name, $field['field_name'])) {
      foreach ($bundles as  $bundle) {

        // If this field is attached to the same node bundle
        if ($filefield_instance = field_info_instance('node', $field_name, $bundle)) {

          // If file extensions for this filefield are limited to zip extension
          if ('zip' == $filefield_instance['settings']['file_extensions']) {
            $filefield_instance_label = $filefield_instance['label'] .' (node/'. $filefield_instance['bundle'] .')';
            $filefield_list[$filefield_instance['id']] = $filefield_instance_label;
          }
        }
      }
    }
  }
  
  return $filefield_list;
}

/**
 * The #value_callback for the file_generic field element.
 */
function _spatial_file_widget_value($element, $input, $form_state) {
  if ($input) {
    // Checkboxes lose their value when empty.
    // If the display field is present make sure its unchecked value is saved.
    $field = field_widget_field($element, $form_state);
    if (empty($input['display'])) {
      $input['display'] = $field['settings']['display_field'] ? 0 : 1;
    }
  }
  else {
    return;
  }

  $fid = 0;

  // Find the current value of this field from the form state.
  $form_state_fid = $form_state['values'];

  foreach ($element['#parents'] as $parent) {
    $form_state_fid = isset($form_state_fid[$parent]) ? $form_state_fid[$parent] : 0;
  }

  if ($element['#extended'] && isset($form_state_fid['fid'])) {
    $fid = $form_state_fid['fid'];
  }
  elseif (is_numeric($form_state_fid)) {
    $fid = $form_state_fid;
  }

  // Process any input and save new uploads.
  if ($input !== FALSE) {
    $return = $input;
    // We depend on the managed file element to handle uploads.
    //  $return = file_managed_file_value($element, $input, $form_state);
    // Load file if the FID has changed to confirm it exists.
    if ($input['fid'] && $verify_file = file_load($input['fid'])) {
      $fid = $verify_file->fid;
    }
  }
  // If there is no input, set the default value.
  else {
    if ($element['#extended']) {
      $default_fid = isset($element['#default_value']['fid']) ? $element['#default_value']['fid'] : 0;
      $return = isset($element['#default_value']) ? $element['#default_value'] : array('fid' => 0);
    }
    else {
      $default_fid = isset($element['#default_value']) ? $element['#default_value'] : 0;
      $return = array('fid' => 0);
    }

    // Confirm that the file exists when used as a default value.
    if ($default_fid && $file = file_load($default_fid)) {
      $fid = $file->fid;
    }
  }
  $return['fid'] = $fid;

  if (empty($return['description'])) {
    $return['description'] = '';
  }
  if (empty($return['display'])) {
    $return['display'] = 1;
  }

  return $return;
}









//----------------------------------------------
// Internal helper functions
//----------------------------------------------

/*
 * This function returns an option list of all the files this widget could reference
 */
function _spatial_file_widget_get_filefield_options($widget_settings, $form_state, $langcode) {
  $file_reference_options = array();
  $file_reference_options[''] = 'Select a file to reference';

  if ($widget_settings['include_only_from_node']) {
    // "Include only from node" is currently the default and only supported choice
    // TODO: To be entity generic, this really ought not to be a node

    // Get the node from the form and the values from the node
    if (isset($form_state['node']) && $node = $form_state['node']) {
      foreach ($widget_settings['allowed_filefields'] as $filefield_instance_id) {
        if ($filefield_instances = field_read_instances(array('id' => $filefield_instance_id))) {
          foreach ($filefield_instances as $filefield_instance) {
            $filefield_name = $filefield_instance['field_name'];
            if (isset($node->$filefield_name) && $filefield_data = $node->$filefield_name) {
              foreach ($filefield_data[$langcode] as $filefield) {
                if (isset($filefield['description']) && $filefield['description']) {
                  $filefield_label = $filefield['description'] .' ('. $filefield['filename'] .')';
                }
                else {
                  $filefield_label = $filefield['filename'];
                }
                $file_reference_options[$filefield_instance['label']][$filefield['fid']] = $filefield_label;
              }
            }
          }
        }
      }
    }
  }
  else {
    // TODO: This is currently not an option, probably will never be supported

    // Pull in all field instaces for the allowed field types
    $filefield_instances = array();
    foreach ($widget_settings['allowed_filefields'] as $filefield_name) {
      // TODO: Foreach field name look up the bundle
      $filefield_definitions = field_info_field($filefield_name);
      foreach ($filefield_definitions['bundles'] as $filefield_entity_type => $filefield_bundles) {
        foreach ($filefield_bundles as $bundle_name) {
          $filefield_instances[] = field_info_instance($filefield_entity_type, $filefield_name, $bundle_name);       
        }
      }
    }

    // TODO: Do something with $filefield_instances to get all these fields (load all values, group by entity optgroups)
  }
  
  return $file_reference_options;
}

/*
 * This function recursively gets all node filefield bundles
 */
function _spatial_file_widget_get_node_bundles($bundles) {
  $return_bundles = array();

  if (isset($bundles['node']) && $bundles['node']) {
    foreach ($bundles['node'] as $bundle) {
      // Entity type is node, so just add the bundle
      $return_bundles[$entity_type][] = $bundle;
    }
  }

  return $return_bundles;
}

