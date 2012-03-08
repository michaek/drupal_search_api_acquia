<?php 

/**
 * Search service class using Acquia's Solr server.
 */
class SearchApiAcquiaService extends SearchApiSolrService {

  /**
   * Create a connection to the Acquia Solr server as configured in $this->options.
   */
  protected function connect() {
    if (!$this->solr) {
      if (!class_exists('Apache_Solr_Service')) {
        throw new Exception(t('SolrPhpClient library not found! Please follow the instructions in search_api_solr/INSTALL.txt for installing the Solr search module.'));
      }
      $this->solr = new SearchApiAcquiaConnection($this->options);
    }
  }

  public function configurationForm(array $form, array &$form_state) {
    $this->options = $this->options + array(
      'host' => 'search.acquia.com',
      'port' => '80',
      'path' => '/solr/'.variable_get('acquia_identifier', ''),
    );
    
    return parent::configurationForm($form, $form_state);
  }

}