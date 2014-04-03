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
	
	_create_companies_data();
	drush_print( 
    dt("Create production companies") 
  );
	
	_create_films_data();
	drush_print( 
    dt("Create films") 
  );
}


function _create_directors_data() {
	// Switch db is expensive, so use following
	// http://drupal.stackexchange.com/questions/88537/connect-drupal-to-external-data-base
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmDirector', 'dir');

  $query->fields('dir', array('slug', 'name'))
  	->orderBy('id', 'ASC');
  
  $result = $query->execute();	

	while($row = $result->fetchAssoc()) {
		_create_node_director($row['slug'], $row['name']);
	}
}


function _create_companies_data() {
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmProductionCompany', 'com');

  $query->fields('com', array('slug', 'name'))
  	->orderBy('id', 'ASC');
  
  $result = $query->execute();	

	while($row = $result->fetchAssoc()) {
		_create_node_company($row['slug'], $row['name']);
	}
}


function _create_films_data() {
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmFilm', 'film');

	$get_row = array(
		'slug', 
		'synopsis_markup',
		'title',
		'country_of_origin',
		'year',
		'notes_markup',
	);
	
  $query->fields('film', $get_row)
  	->orderBy('id', 'ASC');
  
  $result = $query->execute();	

	while($row = $result->fetchAssoc()) {
		_create_node_film(
			$row['slug'],
			$row['synopsis_markup'], 
			$row['title'], 
			$row['country_of_origin'],
			
			$row['year'],
			$row['notes_markup']
		);
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


function _create_node_company($slug, $name) {
	$node = new stdClass();
  $node->type = "company";
  $node->nid = NULL;
  $node->uid = 1;
  
  $node->language = LANGUAGE_NONE;
  $node->title = $name;
  $node->path['alias'] = $slug;
  
  node_save($node);
}


function _create_node_film(
	$slug,
	$synopsis_markup,
	$title,
	$country_of_origin,
	
	$year,
	$notes_markup
	 
) {
	//http://drupal.stackexchange.com/questions/54596/no-body-when-creating-a-node-programatically-with-internationalization

	$node = new stdClass();
  $node->type = "film";
  $node->nid = NULL;
  $node->uid = 1;
  
  $node->language = LANGUAGE_NONE;
  $node->path['alias'] = $slug;
  
  $node->body[$node->language][0] = array('format'=> 'full_html', 'value' => $synopsis_markup);
  
  $node->title = $title;
  $node->field_film_country[$node->language][0]['value'] = $country_of_origin;
  
  $node->field_film_year[$node->language][0]['value'] = $year;
  $node->field_film_notes[$node->language][0]['value'] = $notes_markup;
  
  node_save($node);
}


function _clean_up_nodes() {
	_delete_nodes('director');
	_delete_nodes('company');
	_delete_nodes('film');
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
