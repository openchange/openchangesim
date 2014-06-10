#########################################
#### Automatically create mailboxes #####
#########################################

### define the password to use for all mailboxes
$password = Read-Host "Enter Password" -AsSecureString

### define the template username portion to use
$user = "user"

### define the FQDN
$fqdn = "exchange2007.local"

### database
$database = "First Storage Group\Mailbox Database"

### OrganizationalUnit
$ou = "exchange2007.local/Users"

### define start ($i) and end ($end)
$i = 1
$end = 1000


################## DO NOT EDIT FROM HERE #######################

do {

$upn = $user + $i + "@" + $fqdn
$name = $user + $i

new-mailbox -Password $password -Database $database -UserPrincipalName $upn -Name $name -OrganizationalUnit $ou;
$i++;

} while ($i -le $end)