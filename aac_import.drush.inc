<?php

/*
	Asian Australian Cinema: import data into drupal
*/

/**
 * Implements hook_drush_command().
 */
function aac_import_drush_command() {
  $items['aac_import'] = array(
    'callback' => 'drush_aac_import',
    'description' => dt('Import data drupal'),
    'examples' => array(
      'simple' => dt('drush aac_import'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}

function drush_aac_import() {
  db_set_active('aac_ruby_db');

	$directors_data = _get_directors_data();
	
}


function _get_directors_data() {
	$query = db_select('AacflmDirector', 'dir');
  $query->fields('dir', array('slug', 'name'))
  	->orderBy('id', 'ASC');
  
  $result = $query->execute();	

	while($row = $result->fetchAssoc()) {
		print_r($row);
	}
}
