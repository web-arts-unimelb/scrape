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
	
	$is_created_data = true;
	if($is_created_data) {
		_aac_create_data();
	}
	
	
}


function _aac_create_data() {
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
		'category',
		'title',
		
		'country_of_origin',
		'id',
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
			$row['category'],
			$row['title'],
			
			$row['country_of_origin'],
			$row['id'],
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
	$category_slug, // Only single category in yml data
	$title,
	
	$country_of_origin,
	$film_id,
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
  
  // Category
  $ids = _aac_get_vocabulary_array('film_category');
  $category_name = _aac_slug_to_category($category_slug);
  $category_id = $ids[$category_name];  
  $node->field_film_category[$node->language][0] = array('tid' => $category_id);

  // Tag
  $ids = _aac_get_tag_term_ids($film_id);
  foreach($ids as $term_id) {
  	$node->field_shared_tags[$node->language][] = array('tid' => $term_id);
  }
  
  // Director
  $director_ids = _aac_get_director_ids($film_id);
  foreach($director_ids as $director_id) {
  	$node->field_film_director[$node->language][0] = array('target_id' => $director_id);
	}
  
  // Company
  $company_ids = _aac_get_company_ids($film_id);
  foreach($company_ids as $company_id) {
  	$node->field_film_company[$node->language][0] = array('target_id' => $company_id);
	}
  
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


function _aac_slug_to_category($category_slug) {
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('Category', 'cat');	
	
	$result = $query
		->fields('cat', array('name'))
		->condition('slug', $category_slug, '=')
  	->execute()
		->fetchAssoc();

	$category_name = strtolower($result['name']);

	return $category_name;
}


function _aac_get_tag_term_ids($film_id) {
	$return_array = array();
	$drupal_aac_tag = _aac_get_vocabulary_array("tags");
	
	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('Tag');
	$result = $query
		->fields("Tag", array("phrase", "taggable_id"))
		->condition("taggable_id", $film_id, "=")
		->orderBy("taggable_id", 'ASC')
  	->execute();
  
  foreach($result as $data) {
  	$return_array[] = $drupal_aac_tag[strtolower($data->phrase)];
  }
  
  return $return_array;	
}


function _aac_get_vocabulary_array($voca_name = '') {
	$return_array = array();

	$query = db_select("taxonomy_term_data", "term");
	$query->join("taxonomy_vocabulary", "voca", "term.vid = voca.vid");
	$result = $query
  	->fields("term", array("tid", "name"))
  	->condition("voca.machine_name", $voca_name)
  	->orderBy("tid", 'ASC')
  	->execute();

	foreach($result as $data)
	{
		$return_array[strtolower($data->name)] = $data->tid; 
	}
	
	return $return_array;
}


function _aac_get_director_ids($film_id) {
	$return_array = array();

	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmFilm', 'film');
	$query->join("AacflmDirection", "direction", "film.id = direction.film_id");
	$query->join("AacflmDirector", "dir", "direction.director_id = dir.id");
	$query->condition("film.id", $film_id, "=");
	
	$result = $query
		->fields('dir', array('name'))
		->execute();
	
	foreach($result as $data)
	{
		// Get director id from drupal node
		$title = $data->name;
		$type = 'director';
		$result = db_query("SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", array(":title"=> $title, ":type"=> $type));  
		$nid = $result->fetchField();
		
		$return_array[$title] = $nid; 
	}
	
	return $return_array;
}


function _aac_get_company_ids($film_id) {
	$return_array = array();

	$query = Database::getConnection('default', 'aac_ruby_db')
		->select('AacflmFilm', 'film');
	$query->join("AacflmProduction", "prod", "film.id = prod.film_id");
	$query->join("AacflmProductionCompany", "com", "prod.production_company_id = com.id");
	$query->condition("film.id", $film_id, "=");
	
	$result = $query
		->fields('com', array('name'))
		->execute();
	
	foreach($result as $data)
	{
		// Get director id from drupal node
		$title = $data->name;
		$type = 'company';
		$result = db_query("SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", array(":title"=> $title, ":type"=> $type));  
		$nid = $result->fetchField();
		
		$return_array[$title] = $nid; 
	}
	
	return $return_array;
}
