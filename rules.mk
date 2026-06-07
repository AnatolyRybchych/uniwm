ifeq ($(PLATFORM),windows)
    CC    := gcc
    LD    := gcc
    AR    := ar
    MKDIR := mkdir
    RM    := rmdir /s /q
    GIT   := git
    TOUCH := type nul >
else
    $(error rules.mk: no commands defined for platform '$(PLATFORM)')
endif
