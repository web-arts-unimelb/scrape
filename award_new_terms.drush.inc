<?php
  
  
/**
* Implements hook_drush_command().
*/
function award_new_terms_drush_command()
{
	// When adding new term to the study area taxonomy, if all the checkboxes are selected 
	// in study area, the new term needs to be selected as well.
	// This script will select all terms including the new term.
	$items['award_new_terms'] = array(
		'callback' => 'drush_award_new_terms',
    'description' => dt('When adding new term to the study area taxonomy, if all the checkboxes are selected in study area, the new term needs to be selected as well. This script will select all terms including the new term.'),
    'examples' => array(
      'simple' => dt('drush award_new_terms'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  
	return $items;
}


function drush_award_new_terms()
{
	$new_added_terms_num = 1;
	$award_array = _get_award_nodes(); 
	
	$vid = taxonomy_vocabulary_machine_name_load('study_area')->vid;
	$curr_study_area = taxonomy_get_tree($vid, 0, NULL);
	$curr_study_area_ids = array();
	$curr_study_area_num = count($curr_study_area);	
	
	foreach($curr_study_area as $single_study_area) {
		$curr_study_area_ids[] = $single_study_area->tid;
	}
	
	foreach($award_array as $award_obj) {
		$title = $award_obj->title;
		$used_study_area = $award_obj->field_award_study_area;
		
		$used_study_area_num = count($used_study_area['und']);
		
		if($curr_study_area_num - $used_study_area_num === $new_added_terms_num) {
			foreach($curr_study_area_ids as $curr_study_area_id) {
				$award_obj->field_award_study_area['und'][] = array("tid" => $curr_study_area_id);
			}
			
			node_save($award_obj);
		}
	}
}

function _get_award_nodes() {
	$query = new EntityFieldQuery();
  $result = $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'award')
    ->execute();
  return $nodes = node_load_multiple(array_keys($result['node']));
}

