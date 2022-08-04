<?php
    // Connect to Database
    include("config.php");

    // Write data
    $ten = $_POST["name"];

    $mssv = $_POST["mssv"]; 
    
    $tt = $_POST["trangthai"];

    // update lai database    
    $sql = "update forget set name='$ten' ,id = '$mssv', trangthai=2";

    mysqli_query($conn, $sql);
    mysqli_close($conn);

?>
