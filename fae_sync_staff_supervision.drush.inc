<?php

$lib_path = dirname(__FILE__). "/lib/lib.php";
include_once($lib_path);

$devsite_credential_path = dirname(__FILE__). "/config/devsite_credential.config.php";
include_once($devsite_credential_path);

$this_script_config_path = dirname(__FILE__). "/config/fae_sync_staff_supervision.config.php";
include_once($this_script_config_path);


/**
 * Implements hook_drush_command().
 */
function fae_sync_staff_supervision_drush_command() {
  $items['fae_sync_staff_supervision'] = array(
    'callback' => 'fae_sync_staff_supervision',
    'description' => dt('fae_sync_staff_supervision'),
    'examples' => array(
      'simple' => dt('drush fae_sync_staff_supervision'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}


function fae_sync_staff_supervision() {
  // Now I am sitting within a website

  /*
  // Get all active users
  $result = db_select('users', 'u')
    ->fields('u', array('uid'))
    ->condition('status', 0, '<>')
    ->orderBy('uid', 'ASC')
    ->execute();
  */

  // http://www.findanexpert.unimelb.edu.au/display/person640922

  // http://cc-stage.local/user/206/edit?destination=admin/people

  // if (empty($account->field_fae_resource[LANGUAGE_NONE][0]['url'])) {
 
  // field_data_field_fae_resource 
 
  /* 
  $query = db_select('location', 'l')
  ->fields('l');

$query->join('users', 'u', 'u.uid = l.uid_column_name');

$results = $query->execute();
  */  

  $query = db_select('users', 'u');
  $query->join('field_data_field_fae_resource', 'f', 'u.uid = f.entity_id');

  $result = $query->condition('u.status', 0, '<>')
    ->fields('u', array('uid'))
    ->orderBy('u.uid', 'ASC')
    ->execute();

  foreach($result as $row) {
    $account = user_load($row->uid, TRUE);
    $fae_url = $account->field_fae_resource[LANGUAGE_NONE][0]['url'];

    // Get supervision status from fae (find an expert) website

  }
 
}

