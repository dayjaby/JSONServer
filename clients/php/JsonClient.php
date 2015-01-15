<?php
/*
 * PHP client for BaseX.
 * Works with BaseX 7.0 and later
 *
 * Documentation: http://docs.basex.org/wiki/Clients
 * 
 * (C) BaseX Team 2005-12, BSD License
 */
class Session {
  // class variables.
  var $socket, $info, $buffer, $bpos, $bsize;

  function __construct($h, $p) {
    // create server connection
    $this->socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if(!socket_connect($this->socket, $h, $p)) {
      throw new Exception("Can't communicate with server.");
    }
  }

  public function execute($com) {
    // send command to server
    $len = strlen($com);
    $f = $len & 0xFF;
    socket_write($this->socket, 
      chr(($len)    &0xFF).
      chr(($len>>8) &0xFF).
      chr(($len>>16)&0xFF).
      chr(($len>>24)&0xFF).
      $com
      .chr(0));

    // receive result
    $result = $this->receive();
    return $result;
  }
  
  public function receive() {
    $buf = socket_read($this->socket,4);  
    $len = ord($buf[0]) + 
          (ord($buf[1])<<8) + 
          (ord($buf[2])<<16) +
          (ord($buf[3])<<24);
    return socket_read($this->socket,$len);       
  }
}
?>