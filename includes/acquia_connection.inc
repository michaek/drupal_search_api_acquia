<?php 

/**
 * Extending SearchApiSolrConnection for Acquia authentication.
 */
class SearchApiAcquiaConnection extends SearchApiSolrConnection {

  /**
   * Constructs a Solr connection with an optional HTTP user and password.
   *
   * @param array $options
   *   An array containing construction arguments.
   */
  public function __construct(array $options) {
    parent::__construct($options);
    if ($this->newClient) {
      $this->_httpTransport = new SearchApiAcquiaHttpTransport($this->http_auth);
    }
  }
  
  /**
   * Provide a cookie for authenticating with Acquia.
   */
  protected function _makeHttpRequest($url, $method = 'GET', $headers = array(), $content = '', $timeout = FALSE) {

    list($cookie, $nonce) = _search_api_acquia_auth_cookie($url);
    $headers['Cookie'] = $cookie;
    
    parent::_makeHttpRequest($url, $method, $headers, $content, $timeout);
  }
  
}
