<?php

/**
 * @file
 * Contains functions extracted from acquia_search.module.
 */

/**
 * In case we've enabled the acquia_search module - which we shouldn't -
 * don't cause a fatal error by defining these functions twice.
 */
if (!function_exists('acquia_search_auth_cookie')) {

  /**
   * Modify a solr base url and construct a hmac authenticator cookie.
   *
   * @param $url
   *  The solr url beng requested - passed by reference and may be altered.
   * @param $string
   *  A string - the data to be authenticated, or empty to just use the path
   *  and query from the url to build the authenticator.
   * @param $derived_key
   *  Optional string to supply the derived key.
   *
   * @return
   *  An array containing the string to be added as the content of the
   *  Cookie header to the request and the nonce.
   */
  function acquia_search_auth_cookie(&$url, $string = '', $derived_key = NULL) {
    $uri = parse_url($url);

    // Add a scheme - should always be https if available.
    if (in_array('ssl', stream_get_transports(), TRUE) && !defined('ACQUIA_DEVELOPMENT_NOSSL')) {
      $scheme = 'https://';
      $port = '';
    }
    else {
      $scheme = 'http://';
      $port = (isset($uri['port']) && $uri['port'] != 80) ? ':'. $uri['port'] : '';
    }
    $path = isset($uri['path']) ? $uri['path'] : '/';
    $query = isset($uri['query']) ? '?'. $uri['query'] : '';
    $url = $scheme . $uri['host'] . $port . $path . $query;

    // 32 character nonce.
    $nonce = base64_encode(drupal_random_bytes(24));

    if ($string) {
      $auth_header = acquia_search_authenticator($string, $nonce, $derived_key);
    }
    else {
      $auth_header = acquia_search_authenticator($path . $query, $nonce, $derived_key);
    }
    return array($auth_header, $nonce);
  }

  /**
   * Derive a key for the solr hmac using the information shared with acquia.com.
   */
  function _acquia_search_derived_key() {
    static $derived_key;
    if (!isset($derived_key)) {
      $key = acquia_agent_settings('acquia_key');
      $subscription = acquia_agent_settings('acquia_subscription_data');
      $identifier = acquia_agent_settings('acquia_identifier');
      // We use a salt from acquia.com in key derivation since this is a shared
      // value that we could change on the AN side if needed to force any
      // or all clients to use a new derived key.  We also use a string
      // ('solr') specific to the service, since we want each service using a
      // derived key to have a separate one.
      if (empty($subscription['active']) || empty($key) || empty($identifier)) {
        // Expired or invalid subscription - don't continue.
        $derived_key = '';
      }
      else {
        $salt = isset($subscription['derived_key_salt']) ? $subscription['derived_key_salt'] : '';
        $derivation_string = $identifier . 'solr' . $salt;
        $derived_key =  hash_hmac('sha1', str_pad($derivation_string, 80, $derivation_string), $key);
      }
    }
    return $derived_key;
  }

  /**
   * Creates an authenticator based on a data string and HMAC-SHA1.
   */
  function acquia_search_authenticator($string, $nonce, $derived_key = NULL) {
    if (empty($derived_key)) {
      $derived_key = _acquia_search_derived_key();
    }
    if (empty($derived_key)) {
      // Expired or invalid subscription - don't continue.
      return '';
    }
    else {
      $time = REQUEST_TIME;
      return 'acquia_solr_time='. $time .'; acquia_solr_nonce='. $nonce .'; acquia_solr_hmac='. hash_hmac('sha1', $time . $nonce . $string, $derived_key) .';';
    }
  }

  /**
   * Validate the authenticity of returned data using a nonce and HMAC-SHA1.
   *
   * @return
   *  TRUE or FALSE.
   */
  function acquia_search_valid_response($hmac, $nonce, $string, $derived_key = NULL) {
    if (empty($derived_key)) {
      $derived_key = _acquia_search_derived_key();
    }
    return $hmac == hash_hmac('sha1', $nonce . $string, $derived_key);
  }

  /**
   * Look in the headers and get the hmac_digest out
   * @return string hmac_digest
   *
   */
  function acquia_search_extract_hmac($headers) {
    $reg = array();
    if (is_array($headers)) {
      foreach ($headers as $name => $value) {
        if (strtolower($name) == 'pragma' && preg_match("/hmac_digest=([^;]+);/i", $value, $reg)) {
          return trim($reg[1]);
        }
      }
    }
    return '';
  }

  /**
   * Implementation of hook_apachesolr_modify_query().
   *
   * Possibly alters the query type ('defType') param to edismax.
   */
  function acquia_search_apachesolr_query_alter($query) {
    // @todo - does it make sense to check $caller too?
    if ($query->solr('getId') != ACQUIA_SEARCH_ENVIRONMENT_ID || $query->getParam('qt') || $query->getParam('defType')) {
      // This is a 'mlt' query or something else custom.
      return;
    }
    // Set the qt to edismax if we have keywords, and we always use it, or are
    // using a wildcard (* or ?).
    $keys = $query->getParam('q');
    if ($keys && (($wildcard = preg_match('/\S+[*?]/', $keys)) || variable_get('acquia_search_edismax_default', 0))) {
      $query->addParam('defType', 'edismax');
      if ($wildcard) {
        $keys = preg_replace_callback('/(\S+[*?]\S*)/', '_acquia_search_lower', $keys);
        $query->replaceParam('q', $keys);
      }
    }
  }

  /**
   * Convert to lower-case any keywords containing a wildcard.
   */
  function _acquia_search_lower($matches) {
    return drupal_strtolower($matches[1]);
  }
  
}
