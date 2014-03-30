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
	$aac_ruby_db = array(
		'database' => 'asian_cinema',
		'username' => 'root',
		'password' => '11111111',
		'host' => 'localhost',
		'driver' => 'mysql',
  );
  // replace 'YourDatabaseKey' with something that's unique to your module
  Database::addConnectionInfo('aac_ruby_db_key', 'default', $aac_ruby_db);
  db_set_active('aac_ruby_db_key');

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
