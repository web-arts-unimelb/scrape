<?php

/**
 * Implements hook_drush_command().
 */
function fae_clean_staff_cache_drush_command() {
  $items['fae_clean_staff_cache'] = array(
    'callback' => 'fae_clean_staff_cache',
    'description' => dt('fae_clean_staff_cache'),
    'examples' => array(
      'simple' => dt('drush fae_clean_staff_cache'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}


function fae_clean_staff_cache() {
	$table_name = 'fae_profile_custom_cache';
	$sql = "TRUNCATE TABLE $table_name";
	db_query($sql);
}
