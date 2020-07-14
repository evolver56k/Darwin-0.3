CP=/bin/cp

XCOMPAT_HEADER_DIR = $(DSTROOT)/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders

install: installhdrs

installhdrs:
	mkdir -p $(XCOMPAT_HEADER_DIR)
	mkdir -p $(XCOMPAT_HEADER_DIR)/db
	mkdir -p $(XCOMPAT_HEADER_DIR)/defaults
	mkdir -p $(XCOMPAT_HEADER_DIR)/remote
	mkdir -p $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) db.h $(XCOMPAT_HEADER_DIR)/db
	$(CP) defaults.h $(XCOMPAT_HEADER_DIR)/defaults
	$(CP) NXConnection.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) NXProxy.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) transport.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) NXMethodSignature.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) NXPortPortal.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) private.h $(XCOMPAT_HEADER_DIR)/remote
	$(CP) NXData.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) NXInvalidationNotifier.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) NXLock.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) NXNetNameServer.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) NXPort.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) NXProtocolChecker.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) exceptions.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) reference.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) senderIsInvalid.h $(XCOMPAT_HEADER_DIR)/machkit
	$(CP) LogFunctions.h $(XCOMPAT_HEADER_DIR)/machkit
