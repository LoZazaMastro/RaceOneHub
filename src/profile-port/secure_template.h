#ifndef FM5_SECURE_TEMPLATE_H
#define FM5_SECURE_TEMPLATE_H
static const char fm5_secure_template[] = "<mx:SecureDeviceAssociationTemplate Name=\"SecurePeerTraffic\" InitiatorSocketDescription=\"SecureConsoleSocket\" AcceptorSocketDescription=\"SecureServerSocket\" MultiplayerSessionRequirement=\"Required\">\r\n                <mx:AllowedUsages>\r\n                  <mx:SecureDeviceAssociationUsage Type=\"Default\" />\r\n                </mx:AllowedUsages>\r\n              </mx:SecureDeviceAssociationTemplate>";
#define FM5_MANIFEST_BYTES 11477u
#endif
