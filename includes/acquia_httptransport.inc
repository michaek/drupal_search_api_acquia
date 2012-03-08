<?php

/**
 * @file
 * Contains the SearchApiAcquiaHttpTransport class.
 */

/**
 * Acquia customization of the HTTP transport interface.
 */
class SearchApiAcquiaHttpTransport extends SearchApiSolrHttpTransport {

  /**
   * Helper method for making an HTTP request.
   * Copied from SearchApiSolrHttpTransport and modified to include auth cookie.
   */
  protected function performHttpRequest($method, $url, $timeout, $rawPost = NULL, $contentType = NULL) {
    $options = array(
      'method' => $method,
      'timeout' => $timeout && $timeout > 0 ? $timeout : $this->getDefaultTimeout(),
      'headers' => array(),
    );

    list($cookie, $nonce) = _search_api_acquia_auth_cookie($url);
    $options['headers']['Cookie'] = $cookie;
    
    if ($this->http_auth) {
      $options['headers']['Authorization'] = $this->http_auth;
    }
    if ($timeout) {
      $options['timeout'] = $timeout;
    }
    if ($rawPost) {
      $options['data'] = $rawPost;
    }
    if ($contentType) {
      $options['headers']['Content-Type'] = $contentType;
    }

    $response = drupal_http_request($url, $options);

    $type = isset($response->headers['content-type']) ? $response->headers['content-type'] : 'text/xml';
    $body = isset($response->data) ? $response->data : NULL;
    return new Apache_Solr_HttpTransport_Response($response->code, $type, $body);
  }

}