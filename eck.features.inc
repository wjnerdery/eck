<?php

/**
 * @{
 * ECK Entity Types
 */

/**
 * Implementation of hook_features_export_options().
 */
function eck_entity_type_features_export_options() {
  module_load_include('inc', 'eck', 'eck.entity_type');
  $entity_types = array();
  foreach (entity_get_info() as $entity_type => $info) {
    // @TODO: Ideally, this module should allow us to access ALL entity types,
    // but, we currently only want to check on types created through ECK.
    if (empty($info['module']) || $info['module'] != 'eck') {
      continue;
    }
    $entity_types[$entity_type] = $info['label'];
  }
  return $entity_types;
}



/**
 * Implementation of hook_features_export.
 */
function eck_entity_type_features_export($data, &$export, $module_name = '') {
  $pipe = array();

  foreach ($data as $entity_type) {
    // Export the entity
    $export['features']['eck_entity_type'][$entity_type] = $entity_type;
    $export['dependencies']['eck_entity_type'] = 'eck';
    $export['dependencies']['features'] = 'features';
  }

  return $pipe;
}



/**
 * Implementation of hook_features_export_render().
 */
function eck_entity_type_features_export_render($module, $data, $export = NULL) {
  module_load_include('inc', 'eck', 'eck.entity_type');
  $elements = array(
    'name' => FALSE,
    'label' => TRUE,
    'properties' => FALSE,
    'custom_properties' => FALSE,
  );

  $output = array();
  $output[] = '  $items = array(';
  foreach ($data as $entity_type_name) {
    // @TODO: There are keys/values which need to be features prepared, but this is a good start.
    if ($entity_info = entity_get_info($entity_type_name)) {
      $output[] = "      '{$entity_type_name}' => " . features_var_export($entity_info);
    }
  }
  $output[] = '  );';
  $output[] = '  return $items;';
  return array('eck_entity_type_info' => implode("\n", $output));
}



/**
 * Implementation of hook_features_revert().
 */
function eck_entity_type_features_revert($module) {
  eck_features_rebuild($module);
}



/**
 * Implements of hook_features_rebuild().
 *
 * Rebuilds eck entities from code defaults.
 */
function eck_entity_type_features_rebuild($module) {
  module_load_include('inc', 'eck', 'eck.entity_type');
  if ($default_entities = features_get_default('eck_entity_type', $module)) {
    foreach ($default_entities as $entity_type_name => $entity_type_info) {
      $entity_type = array(
        'name' => $entity_type_name,
        'label' => $entity_type_info['label'],
        'properties' => $entity_type_info['properties'],
        'custom_properties' => $entity_type_info['custom_properties'],
      );
      db_merge('eck_entity_type')
        ->key(array('name' => $entity_type_name))
        ->fields($entity_type)
        ->execute();

      if (!db_table_exists("eck_{$entity_type_name}")) {
        $entity_type['properties'] = unserialize($entity_type['properties']);
        $entity_type['custom_properties'] = unserialize($entity_type['custom_properties']);
        db_create_table("eck_{$entity_type_name}", eck__entity_type__schema((object) $entity_type));
      }
    }

    drupal_get_schema(NULL, TRUE);
    entity_info_cache_clear();
    menu_rebuild();
  }
}

/**
 * @} End of ECK Entity Types.
 */

/**
 * @{
 * ECK Bundles
 */

/**
 * Implementation of hook_features_export_options().
 */
function eck_bundle_features_export_options() {
  module_load_include('inc', 'eck', 'eck.bundle');
  $bundles = array();
  foreach (entity_get_info() as $entity_type => $entity_info) {
    // @TODO: Ideally, all bundles should be accessible to features, but
    // we're currently just targeting those created through ECK.
    if (empty($entity_info['module']) || $entity_info['module'] != 'eck') {
      continue;
    }
    foreach ($entity_info['bundles'] as $bundle_type => $bundle_info) {
      $bundles[$bundle_type] = $bundle_info['label'];
    }
  }
  return $bundles;
}



/**
 * Implementation of hook_features_export.
 */
function eck_bundle_features_export($data, &$export, $module_name = '') {
  $pipe = array();
  $map = features_get_default_map('eck');

  foreach ($data as $type) {
    // Get the entity name of type
    $instances = field_info_instances();
    foreach ($instances as $name => $bundles) {
      if (isset($bundles[$type])) {
        $entity_name = $name;
        break;
      }
    }

    // If this entity type is provided by a different module, add it as a dependency.
    if (isset($map[$entity_name]) && $map[$entity_name] != $module_name) {
      $export['dependencies'][$map[$entity_name]] = $map[$entity_name];
    }
    else {
      // Add a dependency on the ECK entity.
      $pipe['eck'][] = $entity_name;
    }

    // Export the entity type.
    $export['features']['eck_type'][$type] = $type;
    $export['dependencies']['eck'] = 'eck';
    $export['dependencies']['features'] = 'features';

    // Export fields.
    $fields = $instances[$entity_name][$type];
    foreach ($fields as $field) {
      $pipe['field'][] = "{$entity_name}-{$field['bundle']}-{$field['field_name']}";
    }
  }

  return $pipe;
}



/**
 * Implementation of hook_features_export_render().
 */
function eck_bundle_features_export_render($module, $data, $export = NULL) {
  $elements = array(
    'entity' => FALSE,
    'type' => FALSE,
    'label' => TRUE,
  );
  $output = array();
  $output[] = '  $items = array(';

  foreach (entity_get_info() as $entity_type) {
    // @TODO: Can entities exist without bundles?
    if (empty($entity_type['bundles'])) {
      continue;
    }
    foreach ($entity_type['bundles'] as $bundle_type => $bundle_info) {
      $output[] = "      '{$bundle_type}' => " . features_var_export($bundle_info);
    }
  }

  $output[] = '  );';
  $output[] = '  return $items;';

  return array('eck_bundle_type_info' => implode("\n", $output));
}



/**
 * Implementation of hook_features_revert().
 */
function eck_bundle_features_revert($module) {
  eck_type_features_rebuild($module);
}



/**
 * Implements of hook_features_rebuild().
 *
 * Rebuilds eck entities from code defaults.
 */
function eck_bundle_features_rebuild($module) {
  if ($default_types = features_get_default('eck_type', $module)) {
    foreach ($default_types as $type_name => $type_info) {
      $type = array(
        'entity' => $type_info['entity'],
        'type' => $type_name,
        'label' => $type_info['label'],
      );
      db_merge('eck_types')
        ->key(array('type' => $type_name))
        ->fields($type)
        ->execute();
    }

    drupal_get_schema(NULL, TRUE);
    entity_info_cache_clear();
    menu_rebuild();
  }
}

/**
 * @} End of ECK Bundles
 */
