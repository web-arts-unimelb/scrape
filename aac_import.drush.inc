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
	_clean_up_nodes();

	_create_directors_data();
	drush_print( 
    dt("Create directors") 
  );
	
	
	
}


function _create_directors_data() {
	// Switch db is expensive, so use following
	// http://drupal.stackexchange.com/questions/88537/connect-drupal-to-external-data-base
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmDirector', 'dir');

	//$query = db_select('AacflmDirector', 'dir');
  $query->fields('dir', array('slug', 'name'))
  	->orderBy('id', 'ASC');
  
  $result = $query->execute();	

	while($row = $result->fetchAssoc()) {
		_create_node_director($row['slug'], $row['name']);
	}
	
}


function _create_node_director($slug, $name) {
	$node = new stdClass();
  $node->type = "director";
  $node->nid = NULL;
  $node->uid = 1;
  
  $node->language = LANGUAGE_NONE;
  $node->title = $name;
  $node->path['alias'] = $slug;
  
  node_save($node);
}


function _clean_up_nodes() {
	_delete_nodes('director');

}

function _delete_nodes($node_type = '')
{
  if(empty($node_type)) 
  {
    drush_set_error('NO_ARGUMENT', dt('I require node_type'));
    return 1;
  }

  
  // Select the nodes that we want to delete.
  $result = db_select('node', 'n')
    ->fields('n', array('nid'))
    ->condition('type', $node_type, '=')
    ->execute();

  $deleted_count = 0;
  foreach($result as $record) 
  {
    node_delete($record->nid);
    $deleted_count++;
  }

  // Simple debug message so we can see what had been deleted.
  drush_print( 
    dt("node_type: $node_type | $deleted_count nodes of $node_type have been deleted") 
  );
}
