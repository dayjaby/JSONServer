<?php
  require("JsonClient.php");
  $session = new Session("127.0.0.1", 12122);
  echo $session->execute("{\"hello\":\"world\"}");
?>