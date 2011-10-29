CERTSTORE=cacerts.bks

# Check java version.
JAVA_VERSION=`java -version 2>&1 | head -1`
JAVA_VERSION_MINOR=`expr match "$JAVA_VERSION" "java version \"[1-9]\.\([0-9]\).*\""`
if [ $JAVA_VERSION_MINOR -lt 6 ]; then
  echo "java version 1.6 or greater required for keytool usage"
  exit 255
fi

/usr/lib/jvm/jdk1.6.0_10/bin/keytool \
 -storetype BKS \
 -provider org.bouncycastle.jce.provider.BouncyCastleProvider \
 -providerpath bcprov-jdk16-145.jar \
 -storepass changeit \
 -keystore cacerts.bks \
 -list -v > certilist.log
 