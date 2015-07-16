<?php

$lib_path = dirname(__FILE__). "/lib/lib.php";
include_once($lib_path);

$devsite_credential_path = dirname(__FILE__). "/config/devsite_credential.config.php";
include_once($devsite_credential_path);

$this_script_config_path = dirname(__FILE__). "/config/fae_sync_staff_supervision.config.php";
include_once($this_script_config_path);

define('UOM_SPARQL_ENDPOINT', 'https://mrw.unimelb.edu.au/joseki/oracle');

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
	global $base_url;

  my_msg('-start-');
  my_msg('The user page is cached, if you want to see the end result after this script run, clear the user page cache: e.g. http://cc-stage.local/user/32/edit');
	my_msg('Doing website: '. $base_url);
	my_msg('Using php max num: '. PHP_INT_MAX);

	my_msg('Reset all supervision status');
	_reset_all_supervision_status();

  $query = db_select('users', 'u');
  $query->join('field_data_field_fae_resource', 'r', 'u.uid = r.entity_id');
  $query->join('field_data_field_person_supervision', 'super', 'u.uid = super.entity_id');

  $result = $query->condition('u.status', 0, '<>')
    ->fields('u', array('uid'))
    ->fields('r', array('field_fae_resource_url'))
    ->fields('super', array('field_person_supervision_value'))
    ->orderBy('u.uid', 'ASC')
    ->execute();

  $end_point = variable_get('uom_fae_api_endpoint', UOM_SPARQL_ENDPOINT);

  $the_query = "
PREFIX dc:    <http://purl.org/dc/elements/1.1/>
PREFIX rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
PREFIX rdfs:  <http://www.w3.org/2000/01/rdf-schema#>
PREFIX xsd:   <http://www.w3.org/2001/XMLSchema#>
PREFIX owl:   <http://www.w3.org/2002/07/owl#>
PREFIX fn:    <http://www.w3.org/2005/xpath-functions#>
PREFIX ouext: <http://oracle.com/semtech/jena-adaptor/ext/user-def-function#>
PREFIX oext:  <http://oracle.com/semtech/jena-adaptor/ext/function#>
PREFIX ORACLE_SEM_FS_NS: <http://oracle.com/semtech#timeout=10,qid=12345,STRICT_DEFAULT=F,GRAPH_MATCH_UNNAMED=T>
PREFIX fae:   <http://www.findanexpert.unimelb.edu.au/ontology/>
PREFIX vitro: <http://vitro.mannlib.cornell.edu/ns/vitro/0.7#>
PREFIX vitro-public: <http://vitro.mannlib.cornell.edu/ns/vitro/public#>
PREFIX vivo:  <http://vivoweb.org/ontology/core#>
PREFIX bibo:  <http://purl.org/ontology/bibo/>
PREFIX foaf:  <http://xmlns.com/foaf/0.1/>

SELECT
  ?supervisor
WHERE {
  OPTIONAL { <!fae_url> fae:availableForSupervision ?supervisor. }
}";


	$test_counter = 0;
	$test_max = PHP_INT_MAX;
	//$test_max = 1;
  foreach($result as $row) {
		if($test_counter >= $test_max) {
      break;
    }

    /*
      Sample raw query:

      https://mrw.app.unimelb.edu.au/joseki/oracle?query=PREFIX+dc%3A++++%3Chttp%3A%2F%2Fpurl.org%2Fdc%2Felements%2F1.1%2F%3E%0D%0APREFIX+rdf%3A+++%3Chttp%3A%2F%2Fwww.w3.org%2F1999%2F02%2F22-rdf-syntax-ns%23%3E%0D%0APREFIX+rdfs%3A++%3Chttp%3A%2F%2Fwww.w3.org%2F2000%2F01%2Frdf-schema%23%3E%0D%0APREFIX+xsd%3A+++%3Chttp%3A%2F%2Fwww.w3.org%2F2001%2FXMLSchema%23%3E%0D%0APREFIX+owl%3A+++%3Chttp%3A%2F%2Fwww.w3.org%2F2002%2F07%2Fowl%23%3E%0D%0APREFIX+fn%3A++++%3Chttp%3A%2F%2Fwww.w3.org%2F2005%2Fxpath-functions%23%3E%0D%0APREFIX+ouext%3A+%3Chttp%3A%2F%2Foracle.com%2Fsemtech%2Fjena-adaptor%2Fext%2Fuser-def-function%23%3E%0D%0APREFIX+oext%3A++%3Chttp%3A%2F%2Foracle.com%2Fsemtech%2Fjena-adaptor%2Fext%2Ffunction%23%3E%0D%0APREFIX+ORACLE_SEM_FS_NS%3A+%3Chttp%3A%2F%2Foracle.com%2Fsemtech%23timeout%3D100%2Cqid%3D123%3E%0D%0ASELECT+%3Fsc+%3Fc%0D%0AWHERE+%0D%0A++{+%3Fsc+rdfs%3AsubClassOf+%3Fc}&stylesheet=%2Fjoseki%2Fxml-to-html.xsl
    */
    $the_query = t($the_query, array('!fae_url' => $row->field_fae_resource_url));

    //test: for supervision === N
    //$test_fae_url = 'http://www.findanexpert.unimelb.edu.au/individual/person100034';
    //$the_query = t($the_query, array('!fae_url' => test_fae_url));

    $postdata = http_build_query(array('query' => $the_query)). '&output=json';

    $curl = curl_init();
    curl_setopt($curl, CURLOPT_URL, $end_point);
    curl_setopt($curl, CURLOPT_POST, 1);
    curl_setopt($curl, CURLOPT_POSTFIELDS, $postdata);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1); // Basically no output in command line

    // http://stackoverflow.com/questions/2582057/setting-curls-timeout-in-php
    curl_setopt ($curl, CURLOPT_CONNECTTIMEOUT, 0);
    curl_setopt($curl, CURLOPT_TIMEOUT, 1);

    /*
    Good
    {
      "head": {
        "vars": [ "supervisor" ]
      } ,
      "results": {
        "bindings": [
          {
            "supervisor": { "type": "literal" , "value": "Y" }
          }
        ]
      }
    }

    bad
    {
      "head": {
        "vars": [ "supervisor" ]
      } ,
      "results": {
        "bindings": [
          {
            
          }
        ]
      }
    }
    */

    $output = curl_exec($curl);

    // http://php.net/manual/en/function.json-decode.php
    $json_obj = json_decode($output);

    // This can be empty, due to the 'bad' example above
    $supervision = $json_obj->results->bindings[0]->supervisor->value;

    $supervision_status = (!empty($supervision) && $supervision == 'Y') ? 1 : 0;

    $num_updated = db_update('field_data_field_person_supervision')
      ->fields(
          array(
            'field_person_supervision_value' => $supervision_status,
          )
        )
      ->condition('entity_id', $row->uid, '=')
      ->execute(); 


    // NOTE: Update the cache as well.
		// Is it necessary? Because the fae cache is cleaned every week,
		// the effort will be wasted. But the fact is, the supervision field is
		// separated from the fae cache, so we don't need to update the fae cache.
		

    //test
    my_msg("uid: ". $row->uid);
    my_msg("fae_url: ". $row->field_fae_resource_url);
    my_msg("supervision: ". $row->field_person_supervision_value);


    // Joseki end point is bad.
    $time = 10;
    my_msg("Sleep for $time". "s");
    sleep($time);
   
		++$test_counter; 
  }

  my_msg('-end-'); 
}

function _reset_all_supervision_status($supervision_status = 0) {
	$num_updated = db_update('field_data_field_person_supervision')
		->fields(
    	array(
      	'field_person_supervision_value' => $supervision_status,
      )
    )
    ->execute();
}
